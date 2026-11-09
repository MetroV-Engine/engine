#include <boost/ut.hpp>

#include "Engine/Common/Physic/Forces/Dynamics.hpp"
#include "Engine/Common/Physic/Forces/Spring.hpp"
#include "Engine/Common/Physic/Kinematics.hpp"
#include "../../../Support/Approx.hpp"

using namespace boost::ut;
using namespace Engine;
using TestSupport::near;
using V = Math::Vector2d;

namespace {
void test_spring_force() {
    "a stretched spring pulls towards the anchor"_test = [] {
        expect(near(Physic::springForce(V(3, 0), V(0, 0), 1, 10), V(-20, 0), 1e-12));
    };

    "a compressed spring pushes away from the anchor"_test = [] {
        expect(near(Physic::springForce(V(0.5, 0), V(0, 0), 1, 10), V(5, 0), 1e-12));
    };

    "a spring at rest length exerts no force"_test = [] {
        expect(near(Physic::springForce(V(0, 2), V(0, 0), 2, 10), V::zero(), 1e-12));
    };

    "a body on the anchor gets no force"_test = [] {
        expect(Physic::springForce(V(1, 1), V(1, 1), 1, 10) == V::zero());
    };
}

void test_damped_spring_force() {
    "damping only acts along the spring"_test = [] {
        // At rest length, so only damping: -4 * 3 along X; the sideways 7 is ignored.
        expect(near(Physic::dampedSpringForce(V(2, 0), V(0, 0), V(3, 7), 2, 10, 4), V(-12, 0), 1e-12));
    };

    "without damping it matches springForce"_test = [] {
        expect(near(Physic::dampedSpringForce(V(3, 4), V(0, 0), V(1, 1), 2, 10, 0),
                    Physic::springForce(V(3, 4), V(0, 0), 2, 10), 1e-12));
    };
}

void test_critical_damping() {
    "criticalDamping is 2 * sqrt(k * m)"_test = [] {
        expect(Physic::criticalDamping(100.0, 1.0) == 20.0_d);
    };

    "a critically damped spring settles without overshooting"_test = [] {
        const double stiffness = 100.0;
        const double mass = 1.0;
        const double damping = Physic::criticalDamping(stiffness, mass);
        V position(2, 0);
        V velocity;
        bool overshoot = false;

        for (int step = 0; step < 2000; ++step) {
            const V force = Physic::dampedSpringForce(position, V::zero(), velocity, 1, stiffness, damping);
            Physic::integrate(position, velocity, Physic::accelerationFromForce(force, mass), 0.001);
            overshoot = overshoot || position.x < 1 - 1e-6;
        }

        expect(!overshoot);
        expect(near(position.x, 1.0, 1e-3));
    };
}
}

void run_spring_tests() {
    test_spring_force();
    test_damped_spring_force();
    test_critical_damping();
}
