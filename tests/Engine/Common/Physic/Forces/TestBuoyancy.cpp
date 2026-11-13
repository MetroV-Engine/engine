#include <boost/ut.hpp>

#include <cmath>
#include <numbers>
#include <random>

#include "Engine/Common/Physic/Forces/Buoyancy.hpp"
#include "../../../Support/Approx.hpp"

using namespace boost::ut;
using namespace Engine;
using Shapes::Circle;
using Shapes::Polygon;
using Shapes::Rectangle;
using TestSupport::near;
using V = Math::Vector2f;

namespace {
constexpr float Pi = std::numbers::pi_v<float>;
// A pool 10 m wide, its bottom at y = -4, its surface at y = 0.
constexpr Rectangle Pool{{-5.f, -4.f}, {5.f, 0.f}};

/**
 * @brief The oracle: area and centroid of what is inside both the shape and
 * the pool, counted on a grid of 400 x 400 cells over the shape's box. No
 * clipping, no formula: it shares nothing with what it checks.
 */
template <typename Inside>
Physic::SubmergedPart sampled(const Rectangle& bounds, const Inside& inside) {
    constexpr int Cells = 400;
    const V size = bounds.size() / static_cast<float>(Cells);
    double area = 0.0;
    double x = 0.0;
    double y = 0.0;
    for (int i = 0; i < Cells; ++i) {
        for (int j = 0; j < Cells; ++j) {
            const V point = bounds.min + V((i + 0.5f) * size.x, (j + 0.5f) * size.y);
            if (inside(point) && point.x >= Pool.min.x && point.x <= Pool.max.x && point.y >= Pool.min.y && point.y <= Pool.max.y) {
                const double cell = double(size.x) * size.y;
                area += cell;
                x += point.x * cell;
                y += point.y * cell;
            }
        }
    }
    return area > 0.0 ? Physic::SubmergedPart{float(area), V(float(x / area), float(y / area))} : Physic::SubmergedPart{};
}

void test_formulas() {
    "submergedBoxArea, submergedCircleArea and buoyancyForce"_test = [] {
        expect(near(Physic::submergedBoxArea(-1.f, 1.f, 2.f, 0.f), 2.f, 1e-6f));
        expect(Physic::submergedBoxArea(1.f, 2.f, 2.f, 0.f) == 0.f);
        expect(near(Physic::submergedBoxArea(-3.f, -1.f, 2.f, 0.f), 4.f, 1e-6f));
        expect(near(Physic::submergedCircleArea(0.f, 1.f, 0.f), Pi / 2.f, 1e-5f));
        expect(near(Physic::submergedCircleArea(-2.f, 1.f, 0.f), Pi, 1e-5f));
        expect(Physic::submergedCircleArea(2.f, 1.f, 0.f) == 0.f);
        expect(near(Physic::buoyancyForce(2.f, 0.5f, V(0.f, -10.f)), V(0.f, 10.f), 1e-6f));
    };
}

void test_submerged_part() {
    "a box half under the surface: half its area, centered under the surface"_test = [] {
        const Physic::SubmergedPart part = Physic::submergedPart(Rectangle{{-1.f, -0.5f}, {1.f, 0.5f}}, Pool);
        expect(near(part.area, 1.f, 1e-6f));
        expect(near(part.centroid, V(0.f, -0.25f), 1e-6f));
    };

    "a circle cut at its center: half a disc, its centroid 4r / 3pi under the surface"_test = [] {
        const Physic::SubmergedPart part = Physic::submergedPart(Circle{{1.f, 0.f}, 0.6f}, Pool);
        expect(near(part.area, Pi * 0.36f / 2.f, 1e-5f));
        expect(near(part.centroid, V(1.f, -4.f * 0.6f / (3.f * Pi)), 1e-5f));
    };

    "a circle cut by the fluid's side: the exact area within 0.1 %, not a polygon's"_test = [] {
        // Unit disc, the pool's right side at x = 0.5 from its center: the disc minus the segment
        // beyond, acos(0.5) - 0.5 sqrt(0.75). A 32-gon of the same radius would be 0.6 % short.
        const Rectangle pool{{-10.f, -10.f}, {0.5f, 10.f}};
        const float segment = std::acos(0.5f) - 0.5f * std::sqrt(0.75f);
        const Physic::SubmergedPart part = Physic::submergedPart(Circle{{0.f, 0.f}, 1.f}, pool);
        expect(near(part.area, Pi - segment, 1e-3f * Pi)) << part.area;
    };

    "out of the fluid: nothing; all in: the whole shape, at its own centroid"_test = [] {
        expect(Physic::submergedPart(Circle{{0.f, 2.f}, 0.5f}, Pool).area == 0.f);
        expect(Physic::submergedPart(Rectangle{{6.f, -2.f}, {7.f, -1.f}}, Pool).area == 0.f);
        const Polygon triangle{{-1.f, -3.f}, {1.f, -3.f}, {0.f, -1.f}};
        const Physic::SubmergedPart part = Physic::submergedPart(triangle, Pool);
        expect(near(part.area, triangle.area(), 1e-5f));
        expect(near(part.centroid, triangle.centroid(), 1e-5f));
    };

    "random shapes against the sampled oracle: the area and centroid inside the fluid"_test = [] {
        std::mt19937 random(3);
        const auto uniform = [&random](float low, float high) { return std::uniform_real_distribution<float>(low, high)(random); };
        int failures = 0;
        int partly = 0;
        for (int test = 0; test < 60; ++test) {
            // Around the surface and the right wall (x = 5), where cuts happen.
            const V center(uniform(2.f, 7.f), uniform(-2.f, 2.f));
            const float turn = uniform(0.f, 6.28f);
            const float half = uniform(0.3f, 1.5f);
            Physic::SubmergedPart part;
            Physic::SubmergedPart expected;
            float whole = 0.f;
            if (test % 3 == 0) {
                const Circle ball{center, half};
                part = Physic::submergedPart(ball, Pool);
                expected = sampled(ball.boundingBox(), [&ball](const V& p) { return (p - ball.center).lengthSquared() <= ball.radius * ball.radius; });
                whole = Pi * half * half;
            } else {
                const Polygon shape = (test % 3 == 1 ? Polygon::fromRectangle(Rectangle::fromCenter({}, V(half, half * 0.6f)))
                                                     : Polygon{{-half, -half}, {half, -half}, {0.f, half}})
                                          .rotated(turn)
                                          .translated(center);
                part = Physic::submergedPart(shape, Pool);
                expected = sampled(shape.boundingBox(), [&shape](const V& p) { return shape.contains(p); });
                whole = shape.area();
            }
            // The grid is about 1 / 400 of the size: allow a few cells of error.
            const float cell = whole / 400.f;
            const bool area = near(part.area, expected.area, 4.f * cell + 1e-4f);
            const bool centroid = expected.area < 0.05f * whole || near(part.centroid, expected.centroid, 0.02f * half);
            failures += area && centroid ? 0 : 1;
            partly += expected.area > 0.05f * whole && expected.area < 0.95f * whole ? 1 : 0;
        }
        expect(failures == 0_i);
        expect(partly > 15_i) << "the random cases must cut the shapes";
    };
}
}

void run_buoyancy_tests() {
    test_formulas();
    test_submerged_part();
}
