#include <boost/ut.hpp>

#include "Engine/Common/Physic/Forces/Friction.hpp"
#include "../../../Support/Approx.hpp"

using namespace boost::ut;
using namespace Engine;
using TestSupport::near;
using V = Math::Vector2d;

namespace {
// 10 kg resting on flat ground under g = 9.81.
constexpr double Normal = 10.0 * 9.81;

void test_magnitude() {
    "frictionMagnitude is mu * N"_test = [] {
        expect(near(Physic::frictionMagnitude(0.5, Normal), 49.05, 1e-12));
        expect(constant<Physic::frictionMagnitude(0.0, 98.1) == 0.0>);
    };
}

void test_kinetic_friction() {
    "kinetic friction opposes the velocity with strength mu * N"_test = [] {
        const V friction = Physic::kineticFriction(V(3, 4), 0.5, Normal);

        expect(near(friction, V(-0.6, -0.8) * 49.05, 1e-9));
        expect(near(friction.length(), 49.05, 1e-9));
    };

    "a body at rest gets no kinetic friction"_test = [] {
        expect(Physic::kineticFriction(V::zero(), 0.5, Normal) == V::zero());
    };
}

void test_static_friction() {
    "static friction holds while the applied force is at most muStatic * N"_test = [] {
        expect(Physic::staticFrictionHolds(V(50, 0), 0.6, Normal));
        expect(Physic::staticFrictionHolds(V(0, -58.86), 0.6, Normal));
        expect(!Physic::staticFrictionHolds(V(60, 0), 0.6, Normal));
    };
}

void test_friction_force() {
    "a resting body under a small force stays still"_test = [] {
        expect(Physic::frictionForce(V::zero(), V(50, 0), 0.6, 0.5, Normal) == V(-50, 0));
        expect(Physic::frictionForce(V::zero(), V::zero(), 0.6, 0.5, Normal) == V::zero());
    };

    "a resting body under a large force starts sliding"_test = [] {
        expect(near(Physic::frictionForce(V::zero(), V(60, 0), 0.6, 0.5, Normal), V(-49.05, 0), 1e-9));
    };

    "a sliding body gets kinetic friction against its velocity"_test = [] {
        expect(near(Physic::frictionForce(V(0, 2), V(60, 0), 0.6, 0.5, Normal), V(0, -49.05), 1e-9));
    };
}

void test_velocity_after_kinetic_friction() {
    "friction slows a sliding body by mu * N / m * dt"_test = [] {
        const V velocity = Physic::velocityAfterKineticFriction(V(2, 0), 0.5, Normal, 10, 0.1);

        expect(near(velocity, V(2 - 0.4905, 0), 1e-12));
    };

    "friction stops a body instead of reversing it"_test = [] {
        V velocity(2, 0);
        int steps = 0;
        while (velocity != V::zero() && steps < 1000) {
            const V next = Physic::velocityAfterKineticFriction(velocity, 0.5, Normal, 10, 1.0 / 60.0);
            expect(next.x >= 0.0 && next.x < velocity.x);
            velocity = next;
            ++steps;
        }

        // 2 m/s at 4.905 m/s^2 stops in about 0.41 s, so about 25 steps.
        expect(steps == 25_i);
    };

    "a static body is not slowed"_test = [] {
        expect(Physic::velocityAfterKineticFriction(V(2, 0), 0.5, Normal, 0, 1.0) == V(2, 0));
    };
}

void test_friction_force_for_step() {
    // 10 kg, muStatic 0.5 (holds up to 49.05 N), muKinetic 0.3 (29.43 N), steps of 0.1 s.
    constexpr double Mass = 10.0;
    constexpr double Dt = 0.1;
    const auto forStep = [](V velocity, V applied) {
        return Physic::frictionForceForStep(velocity, applied, 0.5, 0.3, Normal, Mass, Dt);
    };
    // Velocity after integrating the step with the friction force added.
    const auto integrated = [](V velocity, V applied, V friction) { return velocity + (applied + friction) * (Dt / Mass); };

    "a resting body held by static friction gets exactly the opposite of the push"_test = [=] {
        const V friction = forStep(V::zero(), V(30, 0));

        expect(near(friction, V(-30, 0), 1e-9));
        expect(near(integrated(V::zero(), V(30, 0), friction), V::zero(), 1e-12));
    };

    "a resting body pushed above muStatic * N slides against muKinetic * N"_test = [=] {
        const V friction = forStep(V::zero(), V(60, 0));

        expect(near(friction, V(-29.43, 0), 1e-9));
        // (60 - 29.43) / 10 * 0.1
        expect(near(integrated(V::zero(), V(60, 0), friction), V(0.3057, 0), 1e-9));
    };

    "a sliding body gets kinetic friction against its velocity"_test = [=] {
        expect(near(forStep(V(0, 2), V::zero()), V(0, -29.43), 1e-9));
    };

    "a body that friction can stop within the step stops exactly, and does not reverse"_test = [=] {
        const V friction = forStep(V(0.1, 0), V::zero());

        // Kinetic friction could remove 0.2943 m/s this step: only the 0.1 left are removed.
        expect(near(friction, V(-10, 0), 1e-9));
        expect(near(integrated(V(0.1, 0), V::zero(), friction), V::zero(), 1e-12));
    };

    "a static body or a step of length 0 gets no friction"_test = [] {
        expect(Physic::frictionForceForStep(V(2, 0), V(5, 0), 0.5, 0.3, Normal, 0.0, Dt) == V::zero());
        expect(Physic::frictionForceForStep(V(2, 0), V(5, 0), 0.5, 0.3, Normal, Mass, 0.0) == V::zero());
    };
}
}

void run_friction_tests() {
    test_magnitude();
    test_kinetic_friction();
    test_static_friction();
    test_friction_force();
    test_velocity_after_kinetic_friction();
    test_friction_force_for_step();
}
