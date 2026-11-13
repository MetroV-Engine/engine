#include <boost/ut.hpp>

#include <cmath>
#include <random>

#include "Engine/Common/Physic/Impact.hpp"
#include "Engine/Common/Physic/Rotation.hpp"
#include "Engine/Common/Shapes/Polygon.hpp"
#include "../../Support/Approx.hpp"

using namespace boost::ut;
using namespace Engine;
using Shapes::Polygon;
using TestSupport::near;
using V = Math::Vector2f;

namespace {
void test_inertia() {
    "moments of inertia of uniform plates about their center"_test = [] {
        // A 2 m by 1 m plate of 6 kg: 6 (4 + 1) / 12 = 2.5.
        expect(near(Physic::boxInertia(6.f, V(1.f, 0.5f)), 2.5f, 1e-6f));
        expect(near(Physic::discInertia(4.f, 0.5f), 0.5f, 1e-6f));
    };

    "polygonInertia: a rectangle as a polygon gets boxInertia, wherever it is"_test = [] {
        const Polygon plate = Polygon::fromRectangle({{3.f, -2.f}, {5.f, -1.f}});
        expect(near(Physic::polygonInertia(6.f, plate), 2.5f, 1e-5f));
    };

    "polygonInertia: a right triangle of legs a and b, about its centroid: m (a² + b²) / 18"_test = [] {
        const Polygon triangle{{0.f, 0.f}, {3.f, 0.f}, {0.f, 2.f}};
        expect(near(Physic::polygonInertia(9.f, triangle), 9.f * (9.f + 4.f) / 18.f, 1e-4f));
    };

    "parallelAxis adds m d², inverseInertia is 0 for what does not turn"_test = [] {
        expect(near(Physic::parallelAxis(2.f, 3.f, V(1.f, 2.f)), 2.f + 3.f * 5.f, 1e-6f));
        expect(near(Physic::inverseInertia(4.f), 0.25f, 1e-7f));
        expect(Physic::inverseInertia(0.f) == 0.f);
        expect(Physic::inverseInertia(1e-7f) == 0.f);
    };
}

void test_polygon_area() {
    "area, centroid and rotated of a polygon"_test = [] {
        const Polygon slope{{0.f, 0.f}, {2.f, 0.f}, {2.f, 2.f}};
        expect(near(slope.area(), 2.f, 1e-6f));
        expect(near(slope.centroid(), V(4.f / 3.f, 2.f / 3.f), 1e-6f));

        // A trapezoid: its centroid is not the average of its vertices.
        const Polygon ramp{{0.f, 0.f}, {3.f, 0.f}, {3.f, 1.f}, {0.f, 2.f}};
        expect(near(ramp.area(), 4.5f, 1e-6f));
        expect(near(ramp.centroid(), V(4.f / 3.f, 7.f / 9.f), 1e-5f));
        expect(!near(ramp.centroid(), ramp.center(), 1e-2f));

        const Polygon turned = slope.rotated(1.f);
        expect(turned.isConvex());
        expect(near(turned.area(), 2.f, 1e-5f));
        expect(near(turned.vertex(1), V(2.f * std::cos(1.f), 2.f * std::sin(1.f)), 1e-6f));
    };
}

void test_kinematics() {
    "a point of a turning body moves at v + w x r; a force at an arm has torque r x F"_test = [] {
        expect(near(Physic::velocityAtPoint(V(1.f, 0.f), 2.f, V(1.f, 0.f)), V(1.f, 2.f), 1e-6f));
        expect(near(Physic::velocityAtPoint(V(0.f, 0.f), 2.f, V(0.f, -0.5f)), V(1.f, 0.f), 1e-6f));
        expect(Physic::torque(V(1.f, 0.f), V(0.f, 3.f)) == 3.f);
        expect(Physic::torque(V(0.f, 1.f), V(0.f, 3.f)) == 0.f) << "along the arm: no torque";
    };

    "integrateRotation: the angular velocity first, then the angle with the new one"_test = [] {
        float angle = 0.f;
        float spin = 1.f;
        Physic::integrateRotation(angle, spin, 2.f, 0.5f);
        expect(spin == 2.f);
        expect(angle == 1.f);
    };
}

void test_impulse_at_point() {
    "at the centers, bounceImpulse with rotation is bounceImpulse without"_test = [] {
        const Physic::BodyPoint<float> a{V(3.f, 0.f), 5.f, 0.5f, 2.f, V(0.f, 0.f)};
        const Physic::BodyPoint<float> b{V(-1.f, 1.f), -3.f, 0.25f, 1.f, V(0.f, 0.f)};
        const V withRotation = Physic::bounceImpulse(a, b, V(1.f, 0.f), 0.6f, 0.f);
        const V without = Physic::bounceImpulse(V(3.f, 0.f), V(-1.f, 1.f), V(1.f, 0.f), 2.f, 4.f, 0.6f, 0.f);
        expect(near(withRotation, without, 1e-5f));
    };

    "hitting the end of a bar spins it and moves it less than hitting its center"_test = [] {
        // A 1 kg bar, I = 1/12 (1 m long): a 1 kg ball hits it at 2 m/s, e = 1.
        const Physic::BodyPoint<float> ball{V(0.f, -2.f) * -1.f, 0.f, 1.f, 0.f, V(0.f, 0.f)};
        const Physic::BodyPoint<float> center{V(0.f, 0.f), 0.f, 1.f, 12.f, V(0.f, 0.f)};
        const Physic::BodyPoint<float> end{V(0.f, 0.f), 0.f, 1.f, 12.f, V(0.5f, 0.f)};
        const V onCenter = Physic::bounceImpulse(ball, center, V(0.f, 1.f), 1.f, 0.f);
        const V onEnd = Physic::bounceImpulse(ball, end, V(0.f, 1.f), 1.f, 0.f);

        expect(near(onCenter, V(0.f, 2.f), 1e-6f));
        // 1/m + r²/I = 1 + 0.25 * 12 = 4 for the bar's end: J = 2 * 2 / (1 + 4).
        expect(near(onEnd, V(0.f, 0.8f), 1e-6f));
        expect(near(Physic::torque(end.arm, onEnd) * end.inverseInertia, 4.8f, 1e-5f)) << "the bar spins";
    };

    "random impacts at a point: momentum and angular momentum are conserved, energy never grows"_test = [] {
        std::mt19937 random(11);
        const auto uniform = [&random](float low, float high) { return std::uniform_real_distribution<float>(low, high)(random); };
        int failures = 0;
        for (int test = 0; test < 2000; ++test) {
            const float massA = uniform(0.2f, 50.f);
            const float massB = uniform(0.2f, 50.f);
            const float inertiaA = uniform(0.05f, 20.f);
            const float inertiaB = uniform(0.05f, 20.f);
            const V centerA(uniform(-2.f, 2.f), uniform(-2.f, 2.f));
            const V centerB(uniform(-2.f, 2.f), uniform(-2.f, 2.f));
            const V point(uniform(-1.f, 1.f), uniform(-1.f, 1.f));
            const float angle = uniform(0.f, 6.2831853f);
            const V normal(std::cos(angle), std::sin(angle));
            Physic::BodyPoint<float> a{V(uniform(-9.f, 9.f), uniform(-9.f, 9.f)), uniform(-5.f, 5.f), 1.f / massA,
                                       1.f / inertiaA, point - centerA};
            Physic::BodyPoint<float> b{V(uniform(-9.f, 9.f), uniform(-9.f, 9.f)), uniform(-5.f, 5.f), 1.f / massB,
                                       1.f / inertiaB, point - centerB};
            const float e = uniform(0.f, 1.f);

            const auto momentum = [&] { return a.velocity * massA + b.velocity * massB; };
            // About the origin: I w + r x m v for each body.
            const auto angularMomentum = [&] {
                return inertiaA * a.angularVelocity + centerA.cross(a.velocity * massA)
                     + inertiaB * b.angularVelocity + centerB.cross(b.velocity * massB);
            };
            const auto energy = [&] {
                return massA * a.velocity.lengthSquared() + inertiaA * a.angularVelocity * a.angularVelocity
                     + massB * b.velocity.lengthSquared() + inertiaB * b.angularVelocity * b.angularVelocity;
            };
            const V momentumBefore = momentum();
            const float angularBefore = angularMomentum();
            const float energyBefore = energy();

            const V impulse = Physic::bounceImpulse(a, b, normal, e, 0.f);
            b.velocity += impulse * b.inverseMass;
            b.angularVelocity += Physic::torque(b.arm, impulse) * b.inverseInertia;
            a.velocity -= impulse * a.inverseMass;
            a.angularVelocity -= Physic::torque(a.arm, impulse) * a.inverseInertia;

            const float scale = 1.f + momentumBefore.length() + std::fabs(angularBefore);
            const bool conserved = near(momentum(), momentumBefore, 1e-3f * scale)
                && near(angularMomentum(), angularBefore, 1e-3f * (scale + 50.f))
                && energy() <= energyBefore * (1.f + 1e-5f) + 1e-3f
                && (b.pointVelocity() - a.pointVelocity()).dot(normal) >= -1e-3f * (1.f + impulse.length());
            failures += conserved ? 0 : 1;
        }
        expect(failures == 0_i);
    };
}
}

void run_rotation_tests() {
    test_inertia();
    test_polygon_area();
    test_kinematics();
    test_impulse_at_point();
}
