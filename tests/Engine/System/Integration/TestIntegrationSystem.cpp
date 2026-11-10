#include <boost/ut.hpp>

#include <algorithm>
#include <cmath>

#include "ECS/ECS.hpp"
#include "Engine/Common/Physic/Forces/Surface.hpp"
#include "Engine/Common/Physic/Kinematics.hpp"
#include "Engine/System/Friction/FrictionSystem.hpp"
#include "Engine/System/Gravity/GravitySystem.hpp"
#include "Engine/System/Integration/IntegrationSystem.hpp"
#include "../../Support/Approx.hpp"

using namespace boost::ut;
using namespace Engine;
using Component::RigidBody;
using Component::SurfaceContact;
using Component::Transform;
using System::FrictionSystem;
using System::GravitySettings;
using System::GravitySystem;
using System::IntegrationSystem;
using TestSupport::near;
using V = Math::Vector2f;

namespace {
RigidBody makeBody(float mass, float gravityScale = 1.f, V velocity = V::zero(), V force = V::zero()) {
    RigidBody body;
    body.mass = mass;
    body.gravityScale = gravityScale;
    body.velocity = velocity;
    body.force = force;
    return body;
}

void test_integration_step() {
    "the force changes the velocity first, then the position moves with it"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;
        systems.addSystem<IntegrationSystem>(world);
        const ECS::Entity body = world.spawnEntity();
        world.addComponent(body, Transform{V::zero()});
        world.addComponent(body, makeBody(2.f, 1.f, V(1.f, 0.f), V(0.f, -20.f)));

        systems.update(world, 0.5);

        // a = (0, -10); v = (1, -5); x = v * 0.5 = (0.5, -2.5).
        expect(near(world.getComponent<RigidBody>(body).velocity, V(1.f, -5.f), 1e-6f));
        expect(near(world.getComponent<Transform>(body).position, V(0.5f, -2.5f), 1e-6f));
    };

    "the forces are cleared after the step"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;
        systems.addSystem<IntegrationSystem>(world);
        const ECS::Entity body = world.spawnEntity();
        world.addComponent(body, Transform{V::zero()});
        world.addComponent(body, makeBody(1.f, 1.f, V::zero(), V(3.f, 4.f)));
        const ECS::Entity withoutTransform = world.spawnEntity();
        world.addComponent(withoutTransform, makeBody(1.f, 1.f, V::zero(), V(3.f, 4.f)));

        systems.update(world, 1.0 / 60.0);

        expect(world.getComponent<RigidBody>(body).force == V::zero());
        expect(world.getComponent<RigidBody>(withoutTransform).force == V::zero());
    };

    "a kinematic body ignores forces but keeps moving with its velocity"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;
        systems.addSystem<IntegrationSystem>(world);
        const ECS::Entity platform = world.spawnEntity("platform");
        world.addComponent(platform, Transform{V::zero()});
        world.addComponent(platform, makeBody(0.f, 1.f, V(2.f, 0.f), V(0.f, -1000.f)));

        systems.update(world, 0.5);

        expect(world.getComponent<RigidBody>(platform).velocity == V(2.f, 0.f));
        expect(near(world.getComponent<Transform>(platform).position, V(1.f, 0.f), 1e-6f));
    };
}

void test_gravity_then_integration() {
    "a body under gravity falls at g, whatever its mass"_test = [] {
        ECS::Registry world;
        world.setResource<GravitySettings>(GravitySettings{V(0.f, -10.f), 1.f});
        ECS::SystemManager systems;
        systems.addSystem<GravitySystem>(world);
        systems.addSystem<IntegrationSystem>(world);
        const ECS::Entity light = world.spawnEntity();
        world.addComponent(light, Transform{V::zero()});
        world.addComponent(light, makeBody(1.f));
        const ECS::Entity heavy = world.spawnEntity();
        world.addComponent(heavy, Transform{V(5.f, 0.f)});
        world.addComponent(heavy, makeBody(500.f));

        for (int step = 0; step < 60; ++step) {
            systems.update(world, 1.0 / 60.0);
        }

        expect(near(world.getComponent<RigidBody>(light).velocity, V(0.f, -10.f), 1e-3f));
        expect(near(world.getComponent<RigidBody>(heavy).velocity, V(0.f, -10.f), 1e-3f));
        // Semi-implicit Euler: 10 * 1 * (1 + 1/60) / 2 = 5.083 m after 1 s.
        expect(near(world.getComponent<Transform>(light).position.y, -5.0833f, 1e-3f));
    };

    "a jump launched with speedToReachHeight peaks at the designed height"_test = [] {
        ECS::Registry world;
        world.setResource<GravitySettings>(GravitySettings{V(0.f, -9.81f), 1.f});
        ECS::SystemManager systems;
        systems.addSystem<GravitySystem>(world);
        systems.addSystem<IntegrationSystem>(world);
        const ECS::Entity player = world.spawnEntity("player");
        world.addComponent(player, Transform{V::zero()});
        world.addComponent(player, makeBody(70.f, 1.f, V(0.f, Physic::speedToReachHeight(3.f, 9.81f))));

        float highest = 0.f;
        for (int step = 0; step < 120; ++step) {
            systems.update(world, 1.0 / 60.0);
            highest = std::max(highest, world.getComponent<Transform>(player).position.y);
        }

        expect(near(highest, 3.f, 0.09f));
    };
}

// Gravity (g = 10, Y up), friction, integration, in that order, at 60 Hz. Nothing holds the
// bodies up yet (that is the collision response's job), so they sink into the surface: the
// tests only look at the motion along it.
struct FrictionScene {
    ECS::Registry world;
    ECS::SystemManager systems;
    ECS::Entity body;

    FrictionScene(V velocity, SurfaceContact contact) {
        world.setResource<GravitySettings>(GravitySettings{V(0.f, -10.f), 1.f});
        systems.addSystem<GravitySystem>(world);
        systems.addSystem<FrictionSystem>(world);
        systems.addSystem<IntegrationSystem>(world);
        body = world.spawnEntity();
        world.addComponent(body, Transform{V::zero()});
        world.addComponent(body, makeBody(1.f, 1.f, velocity));
        world.addComponent(body, contact);
    }

    void step() { systems.update(world, 1.0 / 60.0); }
    V velocity() { return world.getComponent<RigidBody>(body).velocity; }
    V position() { return world.getComponent<Transform>(body).position; }
};

SurfaceContact makeContact(V normal, float staticFriction, float kineticFriction, V surfaceVelocity = V::zero()) {
    SurfaceContact contact;
    contact.normal = normal;
    contact.staticFriction = staticFriction;
    contact.kineticFriction = kineticFriction;
    contact.surfaceVelocity = surfaceVelocity;
    return contact;
}

void test_friction_then_integration() {
    "a sliding body stops after about v^2 / (2 muKinetic g), then stays still"_test = [] {
        FrictionScene scene(V(6.f, 0.f), makeContact(V(0.f, 1.f), 0.6f, 0.5f));

        bool movedBack = false;
        int stepsToStop = -1;
        for (int step = 1; step <= 120; ++step) {
            const float before = scene.position().x;
            scene.step();
            movedBack = movedBack || scene.velocity().x < 0.f || scene.position().x < before;
            if (stepsToStop < 0 && scene.velocity().x == 0.f) {
                stepsToStop = step;
            }
        }

        // 6 m/s at 5 m/s^2: 1.2 s, so 72 steps. Semi-implicit Euler covers
        // 6^2 / (2 * 5) - 6 / 60 / 2 = 3.55 m instead of the continuous 3.6 m.
        expect(!movedBack);
        expect(stepsToStop >= 71_i && stepsToStop <= 72_i);
        expect(near(scene.position().x, 3.55f, 2e-3f));
        expect(scene.velocity().x == 0.f);
    };

    "on a slope too steep for static friction, a body slides down at g (sin - muKinetic cos)"_test = [] {
        const V normal{-0.5f, std::sqrt(3.f) / 2.f};  // 30 degrees, rising to the right
        const V downSlope{-std::sqrt(3.f) / 2.f, -0.5f};
        FrictionScene scene(V::zero(), makeContact(normal, 0.3f, 0.2f));

        for (int step = 0; step < 60; ++step) {
            scene.step();
        }

        // 10 * (0.5 - 0.2 * 0.866) = 3.268 m/s^2, for 1 s.
        expect(near(scene.velocity().dot(downSlope), 3.268f, 1e-3f));
    };

    "on a gentle enough slope, static friction keeps a resting body still"_test = [] {
        const V normal{-0.5f, std::sqrt(3.f) / 2.f};
        FrictionScene scene(V::zero(), makeContact(normal, 0.7f, 0.5f));  // tan(30 degrees) = 0.577 < 0.7

        for (int step = 0; step < 60; ++step) {
            scene.step();
        }

        expect(near(Physic::tangentialComponent(scene.velocity(), normal), V::zero(), 1e-5f));
    };

    "a conveyor belt brings a resting body up to its speed, then carries it"_test = [] {
        FrictionScene scene(V::zero(), makeContact(V(0.f, 1.f), 0.6f, 0.5f, V(3.f, 0.f)));

        for (int step = 0; step < 18; ++step) {
            scene.step();
        }
        // Dragged at muKinetic * g = 5 m/s^2 for 0.3 s.
        expect(near(scene.velocity().x, 1.5f, 1e-4f));

        for (int step = 18; step < 120; ++step) {
            scene.step();
        }
        // Up to the belt's 3 m/s after 0.6 s, then held there by static friction.
        expect(near(scene.velocity().x, 3.f, 1e-4f));
    };
}

// A planet of mass 500 at the origin with G = 1, and one satellite of mass 1
// launched perpendicular to its radius at speedFactor times the circular speed.
struct Orbit {
    ECS::Registry world;
    ECS::SystemManager systems;
    ECS::Entity satellite;

    Orbit(V start, float speedFactor) {
        world.setResource<GravitySettings>(GravitySettings{V::zero(), 1.f});
        systems.addSystem<GravitySystem>(world);
        systems.addSystem<IntegrationSystem>(world);
        const ECS::Entity planet = world.spawnEntity("planet");
        world.addComponent(planet, Transform{V::zero()});
        world.addComponent(planet, Component::GravityAttractor{500.f});
        const float circularSpeed = std::sqrt(500.f / start.length());
        satellite = world.spawnEntity("satellite");
        world.addComponent(satellite, Transform{start});
        world.addComponent(satellite, makeBody(1.f, 1.f, start.normalized().perpendicular() * (circularSpeed * speedFactor)));
    }

    void step() { systems.update(world, 1.0 / 60.0); }
    float distance() { return world.getComponent<Transform>(satellite).position.length(); }
    float energy() {
        const V velocity = world.getComponent<RigidBody>(satellite).velocity;
        return 0.5f * velocity.lengthSquared() - 500.f / distance();
    }
};

void test_orbit_fates() {
    "below escape speed, the satellite stays between its closest and farthest points"_test = [] {
        // speedFactor 0.9 from 5 m: closest point 5 * 0.81 / 1.19 = 3.40 m, farthest 5 m.
        Orbit orbit(V(5.f, 0.f), 0.9f);
        float closest = orbit.distance();
        float farthest = orbit.distance();
        for (int step = 0; step < 60 * 20; ++step) {
            orbit.step();
            closest = std::min(closest, orbit.distance());
            farthest = std::max(farthest, orbit.distance());
        }

        expect(near(closest, 3.40f, 0.1f));
        expect(near(farthest, 5.f, 0.1f));
        expect(orbit.energy() < 0.f);
    };

    "above escape speed, the satellite leaves and never comes back"_test = [] {
        // Escape speed is sqrt(2) = 1.41 times the circular speed.
        Orbit orbit(V(5.f, 0.f), 1.5f);
        expect(orbit.energy() > 0.f);
        float previous = orbit.distance();
        bool cameBack = false;
        for (int step = 0; step < 60 * 20; ++step) {
            orbit.step();
            cameBack = cameBack || (step > 60 && orbit.distance() < previous);
            previous = orbit.distance();
        }

        expect(!cameBack);
        expect(orbit.distance() > 100.f);
        expect(orbit.energy() > 0.f);
    };

    "too slow for an orbit, the satellite reaches the planet's surface"_test = [] {
        // speedFactor 0.4 from 6 m: closest point 6 * 0.16 / 1.84 = 0.52 m, inside a 1 m planet,
        // reached after half an orbit: pi * sqrt(3.26^3 / 500) = 0.83 s.
        Orbit orbit(V(0.f, 6.f), 0.4f);
        int stepsToSurface = -1;
        for (int step = 1; step <= 60 && stepsToSurface < 0; ++step) {
            orbit.step();
            if (orbit.distance() < 1.f) {
                stepsToSurface = step;
            }
        }

        expect(stepsToSurface > 0_i);
        expect(stepsToSurface <= 50_i);
    };

    "dropped without speed, the satellite falls straight in within the free-fall time"_test = [] {
        // Free fall from 5 m to the center: pi / 2 * sqrt(5^3 / (2 * 500)) = 0.555 s, so 34 steps.
        Orbit orbit(V(5.f, 0.f), 0.f);
        int stepsToSurface = -1;
        for (int step = 1; step <= 60 && stepsToSurface < 0; ++step) {
            orbit.step();
            if (orbit.distance() < 1.f) {
                stepsToSurface = step;
            }
            expect(near(orbit.world.getComponent<Transform>(orbit.satellite).position.y, 0.f, 1e-6f));
        }

        expect(stepsToSurface > 25_i);
        expect(stepsToSurface <= 34_i);
    };
}
}

void run_integration_system_tests() {
    test_integration_step();
    test_gravity_then_integration();
    test_friction_then_integration();
    test_orbit_fates();
}
