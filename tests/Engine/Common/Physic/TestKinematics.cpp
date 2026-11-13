#include <boost/ut.hpp>

#include <algorithm>

#include "Engine/Common/Physic/Kinematics.hpp"
#include "Engine/Common/Physic/Forces/Spring.hpp"
#include "../../Support/Approx.hpp"

using namespace boost::ut;
using namespace Engine;
using TestSupport::near;
using V = Math::Vector2d;

namespace {
constexpr double Gravity = 9.81;

void test_integrate_is_semi_implicit() {
    "integrate updates the velocity first, then moves with the new one"_test = [] {
        V position(0, 0);
        V velocity(1, 0);

        Physic::integrate(position, velocity, V(0, -10), 0.5);

        expect(near(velocity, V(1, -5), 1e-12));
        expect(near(position, V(0.5, -2.5), 1e-12));
    };

    "integrate with no acceleration moves in a straight line"_test = [] {
        V position(1, 1);
        V velocity(2, -3);
        for (int step = 0; step < 10; ++step) {
            Physic::integrate(position, velocity, V::zero(), 0.1);
        }

        expect(near(position, V(3, -2), 1e-12));
        expect(velocity == V(2, -3));
    };
}

void test_integrate_keeps_a_spring_stable() {
    "an undamped spring keeps its energy within 10% over 10 s"_test = [] {
        const double stiffness = 100.0;
        V position(1, 0);
        V velocity;
        double lowest = 1.0;
        double highest = 1.0;

        for (int step = 0; step < 600; ++step) {
            const V force = Physic::springForce(position, V::zero(), 0.0, stiffness);
            Physic::integrate(position, velocity, force, 1.0 / 60.0);
            const double energy = (0.5 * velocity.lengthSquared() + 0.5 * stiffness * position.lengthSquared()) / 50.0;
            lowest = std::min(lowest, energy);
            highest = std::max(highest, energy);
        }

        expect(lowest > 0.9_d);
        expect(highest < 1.1_d);
    };
}

void test_closed_form_trajectory() {
    "velocityAtTime and positionAtTime follow a constant acceleration"_test = [] {
        const V gravity(0, -10);

        expect(near(Physic::velocityAtTime(V(2, 10), gravity, 1.0), V(2, 0), 1e-12));
        expect(near(Physic::positionAtTime(V(1, 0), V(2, 10), gravity, 1.0), V(3, 5), 1e-12));
        expect(near(Physic::positionAtTime(V(1, 0), V(2, 10), gravity, 2.0), V(5, 0), 1e-12));
    };
}

void test_jump_formulas() {
    "speedToReachHeight and apexHeight are inverses"_test = [] {
        const double speed = Physic::speedToReachHeight(3.0, Gravity);

        expect(near(Physic::apexHeight(speed, Gravity), 3.0, 1e-12));
        expect(Physic::speedToReachHeight(0.0, Gravity) == 0.0_d);
    };

    "timeToApex is the time at which the vertical speed reaches 0"_test = [] {
        expect(constant<Physic::timeToApex(10.0, 10.0) == 1.0>);
        expect(constant<Physic::apexHeight(10.0, 10.0) == 5.0>);

        const double speed = Physic::speedToReachHeight(3.0, Gravity);
        const double time = Physic::timeToApex(speed, Gravity);
        expect(near(Physic::positionAtTime(V(), V(0, speed), V(0, -Gravity), time), V(0, 3), 1e-12));
    };

    "an integrated jump reaches the designed height within 3% at 60 FPS"_test = [] {
        V position;
        V velocity(0, Physic::speedToReachHeight(3.0, Gravity));
        double highest = 0.0;
        for (int step = 0; step < 120; ++step) {
            Physic::integrate(position, velocity, V(0, -Gravity), 1.0 / 60.0);
            highest = std::max(highest, position.y);
        }

        expect(near(highest, 3.0, 0.09));
    };

    "speedToReachHeightForStep: the integrated jump peaks at the height, at most g dt² / 8 above"_test = [] {
        for (const double rate : {30.0, 60.0, 144.0}) {
            const double dt = 1.0 / rate;
            for (double height = 0.1; height < 10.0; height += 0.37) {
                V position;
                V velocity(0, Physic::speedToReachHeightForStep(height, Gravity, dt));
                double highest = 0.0;
                while (velocity.y > 0.0) {
                    Physic::integrate(position, velocity, V(0, -Gravity), dt);
                    highest = std::max(highest, position.y);
                }

                expect(highest >= height - 1e-9) << "rate" << rate << "height" << height;
                expect(highest <= height + Gravity * dt * dt / 8.0 + 1e-9) << "rate" << rate << "height" << height;
            }
        }
    };

    "speedToReachHeightForStep with no step length is speedToReachHeight"_test = [] {
        expect(near(Physic::speedToReachHeightForStep(2.5, Gravity, 0.0), Physic::speedToReachHeight(2.5, Gravity), 1e-12));
        expect(near(Physic::speedToReachHeightForStep(0.0, Gravity, 1.0 / 60.0), Gravity / 60.0, 1e-12));
    };
}
}

void run_kinematics_tests() {
    test_integrate_is_semi_implicit();
    test_integrate_keeps_a_spring_stable();
    test_closed_form_trajectory();
    test_jump_formulas();
}
