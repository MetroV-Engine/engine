#include <boost/ut.hpp>

#include "Engine/Common/Physic/Forces/Dynamics.hpp"
#include "../../../Support/Approx.hpp"

using namespace boost::ut;
using namespace Engine;
using TestSupport::near;
using V = Math::Vector2d;

namespace {
void test_inverse_mass() {
    "inverseMass is 1 / m, and 0 for a static body"_test = [] {
        expect(constant<Physic::inverseMass(4.0) == 0.25>);
        expect(constant<Physic::inverseMass(0.0) == 0.0>);
        expect(constant<Physic::inverseMass(-1.0) == 0.0>);
        expect(constant<Physic::inverseMass(1e-7) == 0.0>);
    };
}

void test_second_law() {
    "accelerationFromForce and forceForAcceleration are inverses"_test = [] {
        expect(near(Physic::accelerationFromForce(V(10, -20), 5), V(2, -4), 1e-12));
        expect(near(Physic::forceForAcceleration(5, V(2, -4)), V(10, -20), 1e-12));
    };

    "a static body is not accelerated"_test = [] {
        expect(Physic::accelerationFromForce(V(10, -20), 0) == V::zero());
    };
}

void test_impulses() {
    "impulseFromForce is F * dt"_test = [] {
        expect(near(Physic::impulseFromForce(V(10, 0), 0.5), V(5, 0), 1e-12));
    };

    "velocityChangeFromImpulse and impulseForVelocityChange are inverses"_test = [] {
        expect(near(Physic::velocityChangeFromImpulse(V(5, 0), 2.5), V(2, 0), 1e-12));
        expect(near(Physic::impulseForVelocityChange(2.5, V(2, 0)), V(5, 0), 1e-12));
    };

    "an impulse does not move a static body"_test = [] {
        expect(Physic::velocityChangeFromImpulse(V(5, 0), 0) == V::zero());
    };
}
}

void run_dynamics_tests() {
    test_inverse_mass();
    test_second_law();
    test_impulses();
}
