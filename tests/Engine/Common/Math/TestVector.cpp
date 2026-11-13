#include <boost/ut.hpp>

#include <numbers>
#include <sstream>
#include <type_traits>

#include "Engine/Common/Math/Vector.hpp"
#include "../../Support/Approx.hpp"

using namespace boost::ut;
using namespace Engine::Math;
using TestSupport::near;

namespace {
template <typename V>
concept HasLength = requires(V vector) { vector.length(); };

void test_layout() {
    "Vector2 is a plain value of two coordinates"_test = [] {
        expect(constant<std::is_trivially_copyable_v<Vector2f>>);
        expect(constant<sizeof(Vector2f) == 2 * sizeof(float)>);
        expect(constant<sizeof(Vector2d) == 2 * sizeof(double)>);
        expect(constant<Vector2i() == Vector2i(0, 0)>);
    };

    "conversions between coordinate types are explicit"_test = [] {
        expect(constant<!std::is_convertible_v<Vector2i, Vector2f>>);
        expect(constant<Vector2i(Vector2f(1.9f, -1.9f)) == Vector2i(1, -1)>);
        expect(constant<Vector2f(Vector2i(3, -4)) == Vector2f(3.f, -4.f)>);
    };

    "length-based operations only exist for floating-point vectors"_test = [] {
        expect(constant<!HasLength<Vector2i>>);
        expect(constant<HasLength<Vector2f>>);
        expect(constant<HasLength<Vector2d>>);
    };
}

void test_constants() {
    "named constants have the expected coordinates"_test = [] {
        expect(constant<Vector2i::zero() == Vector2i(0, 0)>);
        expect(constant<Vector2i::one() == Vector2i(1, 1)>);
        expect(constant<Vector2i::unitX() == Vector2i(1, 0)>);
        expect(constant<Vector2i::unitY() == Vector2i(0, 1)>);
    };
}

void test_arithmetic() {
    "vector addition, subtraction and negation"_test = [] {
        expect(constant<Vector2i(1, 2) + Vector2i(3, 4) == Vector2i(4, 6)>);
        expect(constant<Vector2i(5, 7) - Vector2i(1, 2) == Vector2i(4, 5)>);
        expect(constant<-Vector2i(1, -2) == Vector2i(-1, 2)>);
    };

    "scalar multiplication and division, on both sides"_test = [] {
        expect(constant<Vector2i(1, 2) * 3 == Vector2i(3, 6)>);
        expect(constant<3 * Vector2i(1, 2) == Vector2i(3, 6)>);
        expect(constant<Vector2i(7, 9) / 2 == Vector2i(3, 4)>);
        expect(near(Vector2d(1, 2) / 4.0, Vector2d(0.25, 0.5), 1e-15));
        expect(constant<Vector2f(2, 3) * 2 == Vector2f(4, 6)>);
    };

    "component-wise multiplication and division"_test = [] {
        expect(constant<Vector2i(2, 3) * Vector2i(4, 5) == Vector2i(8, 15)>);
        expect(constant<Vector2i(8, 15) / Vector2i(4, 5) == Vector2i(2, 3)>);
    };

    "compound assignments modify the vector in place"_test = [] {
        Vector2d v(1, 2);
        v += Vector2d(1, 1);
        v -= Vector2d(0, 2);
        v *= 4.0;
        v /= 2.0;
        expect(near(v, Vector2d(4, 2), 1e-15));
    };
}

void test_products_and_lengths() {
    "dot and cross products"_test = [] {
        expect(constant<Vector2i(1, 2).dot(Vector2i(3, 4)) == 11>);
        expect(constant<Vector2i(1, 0).dot(Vector2i(0, 5)) == 0>);
        expect(constant<Vector2i::unitX().cross(Vector2i::unitY()) == 1>);
        expect(constant<Vector2i::unitY().cross(Vector2i::unitX()) == -1>);
        expect(constant<Vector2i(2, 4).cross(Vector2i(1, 2)) == 0>);
    };

    "lengths and distances"_test = [] {
        expect(constant<Vector2i(3, 4).lengthSquared() == 25>);
        expect(constant<Vector2i(1, 1).distanceSquaredTo(Vector2i(4, 5)) == 25>);
        expect(Vector2d(3, 4).length() == 5.0_d);
        expect(Vector2d(0, 0).distanceTo(Vector2d(3, 4)) == 5.0_d);
    };

    "normalized has length 1, and the zero vector stays zero"_test = [] {
        expect(near(Vector2d(3, 4).normalized(), Vector2d(0.6, 0.8), 1e-12));
        expect(near(Vector2d(-7, 2).normalized().length(), 1.0, 1e-12));
        expect(Vector2d::zero().normalized() == Vector2d::zero());
        expect(Vector2d(1e-7, 0).normalized() == Vector2d::zero());
    };
}

void test_rotations_and_angles() {
    constexpr double pi = std::numbers::pi;

    "perpendicular turns by +90 degrees"_test = [] {
        expect(constant<Vector2i(1, 0).perpendicular() == Vector2i(0, 1)>);
        expect(constant<Vector2i(0, 1).perpendicular() == Vector2i(-1, 0)>);
    };

    "rotated turns by the given angle"_test = [] {
        expect(near(Vector2d(1, 0).rotated(pi / 2), Vector2d(0, 1), 1e-12));
        expect(near(Vector2d(1, 0).rotated(pi), Vector2d(-1, 0), 1e-12));
        expect(near(Vector2d(2, 3).rotated(0.7).length(), Vector2d(2, 3).length(), 1e-12));
    };

    "angle and angleTo"_test = [] {
        expect(near(Vector2d(0, 1).angle(), pi / 2, 1e-12));
        expect(near(Vector2d(-1, 0).angle(), pi, 1e-12));
        expect(Vector2d::zero().angle() == 0.0_d);
        expect(near(Vector2d(1, 0).angleTo(Vector2d(0, -1)), -pi / 2, 1e-12));
        expect(near(Vector2d(1, 0).angleTo(Vector2d(0, 1)), pi / 2, 1e-12));
    };
}

void test_geometric_operations() {
    "projectedOnto keeps the part along the target"_test = [] {
        expect(near(Vector2d(3, 4).projectedOnto(Vector2d(10, 0)), Vector2d(3, 0), 1e-12));
        expect(Vector2d(3, 4).projectedOnto(Vector2d::zero()) == Vector2d::zero());
    };

    "reflected bounces off a surface"_test = [] {
        expect(constant<Vector2f(1, -1).reflected(Vector2f(0, 1)) == Vector2f(1, 1)>);
        expect(constant<Vector2f(-2, 0).reflected(Vector2f(1, 0)) == Vector2f(2, 0)>);
    };

    "clampedLength only shortens longer vectors"_test = [] {
        expect(Vector2d(3, 4).clampedLength(10) == Vector2d(3, 4));
        expect(near(Vector2d(3, 4).clampedLength(1), Vector2d(0.6, 0.8), 1e-12));
        expect(Vector2d::zero().clampedLength(0) == Vector2d::zero());
    };

    "approxEqual compares each coordinate within a tolerance"_test = [] {
        expect(Vector2d(1, 1).approxEqual(Vector2d(1 + 1e-6, 1 - 1e-6)));
        expect(!Vector2d(0, 0).approxEqual(Vector2d(0, 1e-3)));
        expect(Vector2d(0, 0).approxEqual(Vector2d(0, 1e-3), 1e-2));
    };
}

void test_free_functions() {
    "lerp interpolates and extrapolates"_test = [] {
        expect(constant<lerp(Vector2f(0, 0), Vector2f(10, 20), 0.5) == Vector2f(5, 10)>);
        expect(constant<lerp(Vector2d(0, 0), Vector2d(1, 1), 2.0) == Vector2d(2, 2)>);
    };

    "moveTowards moves by at most maxDistance"_test = [] {
        expect(near(moveTowards(Vector2d(0, 0), Vector2d(3, 4), 2.5), Vector2d(1.5, 2), 1e-12));
        expect(moveTowards(Vector2d(0, 0), Vector2d(3, 4), 6.0) == Vector2d(3, 4));
    };

    "damp approaches the target by the frame-rate independent factor"_test = [] {
        expect(near(damp(Vector2d(0, 0), Vector2d(10, 0), 3.0, 1.0),
                    Vector2d(10 * (1 - std::exp(-3.0)), 0), 1e-12));
    };

    "min, max and abs work per coordinate"_test = [] {
        expect(constant<min(Vector2i(1, 5), Vector2i(3, 2)) == Vector2i(1, 2)>);
        expect(constant<max(Vector2i(1, 5), Vector2i(3, 2)) == Vector2i(3, 5)>);
        expect(abs(Vector2i(-3, 4)) == Vector2i(3, 4));
        expect(abs(Vector2f(-3.5f, 4)) == Vector2f(3.5f, 4));
    };
}

void test_stream_output() {
    "operator<< writes (x, y)"_test = [] {
        std::ostringstream out;
        out << Vector2i(1, -2) << ' ' << Vector2f(0.5f, 2);
        expect(out.str() == "(1, -2) (0.5, 2)");
    };
}
}

void run_vector_tests() {
    test_layout();
    test_constants();
    test_arithmetic();
    test_products_and_lengths();
    test_rotations_and_angles();
    test_geometric_operations();
    test_free_functions();
    test_stream_output();
}
