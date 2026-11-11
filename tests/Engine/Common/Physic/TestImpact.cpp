#include <boost/ut.hpp>

#include <cmath>
#include <numbers>
#include <random>

#include "Engine/Common/Collision/Collide.hpp"
#include "Engine/Common/Physic/Forces/Dynamics.hpp"
#include "Engine/Common/Physic/Impact.hpp"
#include "../../Support/Approx.hpp"

using namespace boost::ut;
using namespace Engine;
using TestSupport::near;
using V = Math::Vector2d;

namespace {
constexpr V Up{0, 1};
constexpr V Right{1, 0};

// Velocities after the impact: B gets the impulse, A the opposite.
struct After {
    V a;
    V b;
};

After bounced(V velocityA, V velocityB, V normal, double massA, double massB, double restitution, double restingSpeed = 0.0) {
    const V impulse = Physic::bounceImpulse(velocityA, velocityB, normal, massA, massB, restitution, restingSpeed);
    return {velocityA - Physic::velocityChangeFromImpulse(impulse, massA), velocityB + Physic::velocityChangeFromImpulse(impulse, massB)};
}

double kineticEnergy(V velocity, double mass) { return 0.5 * mass * velocity.dot(velocity); }

void test_combined_restitution() {
    "two materials bounce like the bouncier one"_test = [] {
        expect(constant<Physic::combinedRestitution(0.2, 0.9) == 0.9>);
        expect(constant<Physic::combinedRestitution(0.9, 0.2) == 0.9>);
    };
}

void test_closing_speed() {
    "closingSpeed is positive when the bodies approach, negative when they part, 0 when they slide"_test = [] {
        // A below, B above: the normal points up, from A to B.
        expect(constant<Physic::closingSpeed(V(0, 0), V(0, -3), Up) == 3.0>);
        expect(constant<Physic::closingSpeed(V(0, 0), V(0, 3), Up) == -3.0>);
        expect(constant<Physic::closingSpeed(V(0, 0), V(5, 0), Up) == 0.0>);
        expect(constant<Physic::closingSpeed(V(0, 2), V(0, -3), Up) == 5.0>);
    };
}

void test_ball_on_the_floor() {
    // The floor is A, static (mass 0); the ball is B, above it: the normal points up.
    "a ball hitting the floor at 5 m/s leaves it at e * 5 m/s, and the floor does not move"_test = [] {
        const After after = bounced(V::zero(), V(0, -5), Up, 0.0, 1.0, 0.5);

        expect(near(after.b, V(0, 2.5), 1e-12));
        expect(after.a == V::zero());
    };

    "e = 0 stops the fall, and keeps the speed along the floor"_test = [] {
        expect(near(bounced(V::zero(), V(3, -5), Up, 0.0, 1.0, 0.0).b, V(3, 0), 1e-12));
    };

    "e = 1 sends it back at the same speed, whatever its mass"_test = [] {
        expect(near(bounced(V::zero(), V(0, -5), Up, 0.0, 1.0, 1.0).b, V(0, 5), 1e-12));
        expect(near(bounced(V::zero(), V(0, -5), Up, 0.0, 70.0, 1.0).b, V(0, 5), 1e-12));
    };

    "under the resting speed it does not bounce, so a ball at rest does not hop"_test = [] {
        // At rest on the ground, gravity pulls it in by g dt = 0.16 m/s every step at 60 Hz.
        expect(near(bounced(V::zero(), V(0, -0.16), Up, 0.0, 1.0, 0.8, 1.0).b, V::zero(), 1e-12));
        // Above it, it bounces.
        expect(near(bounced(V::zero(), V(0, -5), Up, 0.0, 1.0, 0.8, 1.0).b, V(0, 4), 1e-12));
    };

    "a ball leaving the floor, or sliding on it, gets nothing"_test = [] {
        expect(Physic::bounceImpulse(V::zero(), V(0, 2), Up, 0.0, 1.0, 0.5, 0.0) == V::zero());
        expect(Physic::bounceImpulse(V::zero(), V(4, 0), Up, 0.0, 1.0, 0.5, 0.0) == V::zero());
    };

    "on a static platform moving up, the ball leaves at the platform's speed plus e times the impact speed"_test = [] {
        // Platform up at 2 m/s, ball down at 3 m/s: they meet at 5 m/s, and part at 0.5 * 5 = 2.5 m/s.
        const After after = bounced(V(0, 2), V(0, -3), Up, 0.0, 1.0, 0.5);

        expect(near(after.b, V(0, 2 + 2.5), 1e-12));
        expect(after.a == V(0, 2));
    };

    "two static bodies get nothing"_test = [] {
        expect(Physic::bounceImpulse(V(0, 1), V(0, -1), Up, 0.0, 0.0, 0.5, 0.0) == V::zero());
    };
}

void test_two_bodies() {
    "equal masses meeting head on with e = 1 swap their velocities"_test = [] {
        const After after = bounced(V(1, 0), V(-1, 0), Right, 1.0, 1.0, 1.0);

        expect(near(after.a, V(-1, 0), 1e-12));
        expect(near(after.b, V(1, 0), 1e-12));
    };

    "a body hitting a resting one of the same mass with e = 1 stops dead, like a Newton's cradle"_test = [] {
        const After after = bounced(V(2, 0), V::zero(), Right, 1.0, 1.0, 1.0);

        expect(near(after.a, V::zero(), 1e-12));
        expect(near(after.b, V(2, 0), 1e-12));
    };

    "with e = 0 they move on together, at the speed of their center of mass"_test = [] {
        // (1 kg * 4 m/s + 3 kg * 0) / 4 kg = 1 m/s.
        const After after = bounced(V(4, 0), V::zero(), Right, 1.0, 3.0, 0.0);

        expect(near(after.a, V(1, 0), 1e-12));
        expect(near(after.b, V(1, 0), 1e-12));
    };
}

void test_separation() {
    "a body on a static floor moves up by the whole depth, exactly"_test = [] {
        const Physic::Separation<double> moves = Physic::separation(Up, 0.3, 0.0, 2.0);

        expect(moves.first == V::zero());
        expect(moves.second == V(0, 0.3));
    };

    "equal masses share the depth half and half"_test = [] {
        const Physic::Separation<double> moves = Physic::separation(Up, 0.3, 5.0, 5.0);

        expect(moves.first == V(0, -0.15));
        expect(moves.second == V(0, 0.15));
    };

    "a body three times as heavy moves a third as far"_test = [] {
        const Physic::Separation<double> moves = Physic::separation(Right, 0.4, 3.0, 1.0);

        expect(near(moves.first, V(-0.1, 0), 1e-12));
        expect(near(moves.second, V(0.3, 0), 1e-12));
    };

    "two static bodies do not move"_test = [] {
        const Physic::Separation<double> moves = Physic::separation(Up, 0.3, 0.0, 0.0);

        expect(moves.first == V::zero());
        expect(moves.second == V::zero());
    };
}

void test_random_impacts() {
    "on random impacts: parting at e times the closing speed, momentum kept, no energy created"_test = [] {
        std::mt19937 generator(20261111);
        const auto uniform = [&](double low, double high) { return std::uniform_real_distribution<double>(low, high)(generator); };
        int approaching = 0;
        for (int i = 0; i < 10000; ++i) {
            const double angle = uniform(0.0, 2.0 * std::numbers::pi);
            const V normal(std::cos(angle), std::sin(angle));
            const V tangent(-normal.y, normal.x);
            const V velocityA(uniform(-20.0, 20.0), uniform(-20.0, 20.0));
            const V velocityB(uniform(-20.0, 20.0), uniform(-20.0, 20.0));
            const bool staticA = uniform(0.0, 1.0) < 0.2;  // a wall or a moving platform
            const double massA = staticA ? 0.0 : uniform(0.1, 100.0);
            const double massB = uniform(0.1, 100.0);
            const double e = uniform(0.0, 1.0);
            const double closing = Physic::closingSpeed(velocityA, velocityB, normal);

            const After after = bounced(velocityA, velocityB, normal, massA, massB, e);

            if (closing <= 0.0) {
                expect(after.a == velocityA && after.b == velocityB);
                continue;
            }
            ++approaching;
            expect(near(Physic::closingSpeed(after.a, after.b, normal), -e * closing, 1e-9));
            expect(near(after.a.dot(tangent), velocityA.dot(tangent), 1e-9));
            expect(near(after.b.dot(tangent), velocityB.dot(tangent), 1e-9));
            if (staticA) {
                expect(after.a == velocityA);
                continue;
            }
            const double scale = massA * velocityA.length() + massB * velocityB.length();
            expect(near(after.a * massA + after.b * massB, velocityA * massA + velocityB * massB, 1e-12 * scale));
            const double before = kineticEnergy(velocityA, massA) + kineticEnergy(velocityB, massB);
            const double now = kineticEnergy(after.a, massA) + kineticEnergy(after.b, massB);
            const double reducedMass = 1.0 / (1.0 / massA + 1.0 / massB);
            expect(now <= before * (1.0 + 1e-12));
            expect(near(before - now, (1.0 - e * e) / 2.0 * closing * closing * reducedMass, 1e-9 * before));
        }
        expect(approaching > 4000_i);
    };

    "on random overlaps: the moves leave the bodies touching, and keep their center of mass in place"_test = [] {
        std::mt19937 generator(20261112);
        const auto uniform = [&](double low, double high) { return std::uniform_real_distribution<double>(low, high)(generator); };
        for (int i = 0; i < 10000; ++i) {
            const double angle = uniform(0.0, 2.0 * std::numbers::pi);
            const V normal(std::cos(angle), std::sin(angle));
            const double depth = uniform(0.0, 2.0);
            const double massA = uniform(0.1, 100.0), massB = uniform(0.1, 100.0);

            const Physic::Separation<double> moves = Physic::separation(normal, depth, massA, massB);

            expect(near(moves.second - moves.first, normal * depth, 1e-12));
            expect(near(moves.first * massA + moves.second * massB, V::zero(), 1e-12 * (massA + massB)));
        }
    };
}

void test_with_collide() {
    // The same normal convention as Collision::Contact: from the first shape towards the second.
    using Collision::Rectangle;
    using Vf = Math::Vector2f;

    "a box sunk into a static floor is put back on it, touching, and bounces up"_test = [] {
        const Rectangle floor{Vf(-5.f, -1.f), Vf(5.f, 0.f)};
        const Rectangle box = Rectangle::fromCenter(Vf(0.f, 0.3f), Vf(0.5f, 0.5f));  // 0.2 m into the floor
        const auto contact = Collision::collide(floor, box);
        expect(fatal(contact.has_value()));

        const Physic::Separation<float> moves = Physic::separation(contact->normal, contact->depth, 0.f, 1.f);
        const auto after = Collision::collide(floor, box.translated(moves.second));
        const Vf velocity = Vf(2.f, -4.f) + Physic::bounceImpulse(Vf::zero(), Vf(2.f, -4.f), contact->normal, 0.f, 1.f, 0.5f, 1.f);

        expect(moves.first == Vf::zero());
        expect(fatal(after.has_value()));
        expect(after->depth == 0.f);  // touching, not overlapping
        expect(near(velocity, Vf(2.f, 2.f), 1e-6f));
    };

    "two boxes pushed apart end up touching, the lighter one moved three times as far"_test = [] {
        const Rectangle a = Rectangle::fromCenter(Vf(0.f, 0.f), Vf(0.5f, 0.5f));    // 3 kg
        const Rectangle b = Rectangle::fromCenter(Vf(0.6f, 0.f), Vf(0.5f, 0.5f));   // 1 kg, 0.4 m into a
        const auto contact = Collision::collide(a, b);
        expect(fatal(contact.has_value()));

        const Physic::Separation<float> moves = Physic::separation(contact->normal, contact->depth, 3.f, 1.f);
        const auto after = Collision::collide(a.translated(moves.first), b.translated(moves.second));

        expect(near(moves.first, Vf(-0.1f, 0.f), 1e-6f));
        expect(near(moves.second, Vf(0.3f, 0.f), 1e-6f));
        expect(fatal(after.has_value()));
        expect(near(after->depth, 0.f, 1e-6f));
    };
}
}

void run_impact_tests() {
    test_combined_restitution();
    test_closing_speed();
    test_ball_on_the_floor();
    test_two_bodies();
    test_separation();
    test_random_impacts();
    test_with_collide();
}
