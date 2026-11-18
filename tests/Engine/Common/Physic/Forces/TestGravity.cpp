#include <boost/ut.hpp>

#include "Engine/Common/Physic/Forces/Gravity.hpp"
#include "../../../Support/Approx.hpp"

using namespace boost::ut;
using namespace Engine;
using TestSupport::near;
using V = Math::Vector2d;

namespace {
void test_uniform_gravity() {
    "weight is m * g * gravityScale"_test = [] {
        const V gravity(0, -Physic::StandardGravity<double>);

        expect(near(Physic::weight(70, gravity), V(0, -70 * 9.80665), 1e-9));
        expect(near(Physic::weight(2, gravity, 2.0), V(0, -4 * 9.80665), 1e-9));
        expect(Physic::weight(70, gravity, 0.0) == V::zero());
    };

    "weight follows the direction of the gravity vector"_test = [] {
        expect(near(Physic::weight(1, V(0, 9.81)), V(0, 9.81), 1e-12));
    };
}

void test_universal_gravitation_magnitude() {
    "gravitationalForceMagnitude is G * mA * mB / d^2"_test = [] {
        const double earthMoon = Physic::gravitationalForceMagnitude(5.972e24, 7.342e22, 3.844e8);
        expect(near(earthMoon, 1.98049e20, 1e16));
        expect(Physic::gravitationalForceMagnitude(2.0, 5.0, 5.0, 1.0) == 0.4_d);
    };

    "bodies at the same place attract with a force of 0"_test = [] {
        expect(Physic::gravitationalForceMagnitude(1.0, 1.0, 0.0) == 0.0_d);
    };
}

void test_universal_gravitation_vector() {
    "the force on A points towards B, with the expected strength"_test = [] {
        const V force = Physic::gravitationalForce(V(0, 0), 2.0, V(3, 4), 5.0, 1.0);

        expect(near(force, V(0.6, 0.8) * 0.4, 1e-12));
        expect(near(force.length(), Physic::gravitationalForceMagnitude(2.0, 5.0, 5.0, 1.0), 1e-12));
    };

    "the forces on A and B are opposite (Newton's third law)"_test = [] {
        const V onA = Physic::gravitationalForce(V(1, 2), 3.0, V(-4, 6), 7.0, 1.0);
        const V onB = Physic::gravitationalForce(V(-4, 6), 7.0, V(1, 2), 3.0, 1.0);

        expect(near(onA, -onB, 1e-12));
    };

    "coincident bodies get a zero force"_test = [] {
        expect(Physic::gravitationalForce(V(1, 1), 1.0, V(1, 1), 1.0) == V::zero());
    };

    "gravitationalAcceleration gives the Earth's surface gravity"_test = [] {
        const V acceleration = Physic::gravitationalAcceleration(V(0, 6.371e6), V(0, 0), 5.972e24);

        expect(acceleration.x == 0.0_d);
        expect(near(acceleration.y, -9.82, 0.01));
    };
}
}

void run_gravity_tests() {
    test_uniform_gravity();
    test_universal_gravitation_magnitude();
    test_universal_gravitation_vector();
}
