#include <boost/ut.hpp>

#include "ECS/ECS.hpp"
#include "Engine/Common/Physic/Forces/Gravity.hpp"
#include "Engine/System/Physics/Gravity/GravitySystem.hpp"
#include "../../../Support/Approx.hpp"

using namespace boost::ut;
using namespace Engine;
using Component::GravityAttractor;
using Component::RigidBody;
using Component::Transform;
using System::GravitySettings;
using System::GravitySystem;
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

void test_init_sets_default_settings() {
    "init sets the default GravitySettings when none was set"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;

        systems.addSystem<GravitySystem>(world);

        expect(world.hasResource<GravitySettings>());
        expect(world.getResource<GravitySettings>().gravity == V(0.f, -Physic::StandardGravity<float>));
    };

    "init keeps the GravitySettings already set"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;
        world.setResource<GravitySettings>(GravitySettings{V(0.f, 3.f), 1.f});

        systems.addSystem<GravitySystem>(world);

        expect(world.getResource<GravitySettings>().gravity == V(0.f, 3.f));
    };
}

void test_uniform_gravity() {
    "every body receives its weight m * g * gravityScale"_test = [] {
        ECS::Registry world;
        world.setResource<GravitySettings>(GravitySettings{V(0.f, -10.f), 1.f});
        ECS::SystemManager systems;
        systems.addSystem<GravitySystem>(world);
        const ECS::Entity normal = world.spawnEntity();
        world.addComponent(normal, makeBody(2.f));
        const ECS::Entity heavy = world.spawnEntity();
        world.addComponent(heavy, makeBody(2.f, 3.f));

        systems.update(world, 1.0 / 60.0);

        expect(world.getComponent<RigidBody>(normal).force == V(0.f, -20.f));
        expect(world.getComponent<RigidBody>(heavy).force == V(0.f, -60.f));
    };

    "gravity adds to the forces already received"_test = [] {
        ECS::Registry world;
        world.setResource<GravitySettings>(GravitySettings{V(0.f, -10.f), 1.f});
        ECS::SystemManager systems;
        systems.addSystem<GravitySystem>(world);
        const ECS::Entity body = world.spawnEntity();
        world.addComponent(body, makeBody(1.f, 1.f, V::zero(), V(5.f, 0.f)));

        systems.update(world, 1.0 / 60.0);

        expect(world.getComponent<RigidBody>(body).force == V(5.f, -10.f));
    };

    "a body with gravityScale 0 or mass 0 receives no weight"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;
        systems.addSystem<GravitySystem>(world);
        const ECS::Entity floating = world.spawnEntity();
        world.addComponent(floating, makeBody(5.f, 0.f));
        const ECS::Entity kinematic = world.spawnEntity();
        world.addComponent(kinematic, makeBody(0.f));

        systems.update(world, 1.0 / 60.0);

        expect(world.getComponent<RigidBody>(floating).force == V::zero());
        expect(world.getComponent<RigidBody>(kinematic).force == V::zero());
    };
}

void test_attractors() {
    "an attractor pulls a body towards it with G * mA * mB / d^2"_test = [] {
        ECS::Registry world;
        world.setResource<GravitySettings>(GravitySettings{V::zero(), 1.f});
        ECS::SystemManager systems;
        systems.addSystem<GravitySystem>(world);
        const ECS::Entity planet = world.spawnEntity("planet");
        world.addComponent(planet, Transform{V(3.f, 4.f)});
        world.addComponent(planet, GravityAttractor{100.f});
        const ECS::Entity body = world.spawnEntity();
        world.addComponent(body, Transform{V::zero()});
        world.addComponent(body, makeBody(2.f));

        systems.update(world, 1.0 / 60.0);

        // 1 * 2 * 100 / 25 = 8 N, towards (0.6, 0.8).
        expect(near(world.getComponent<RigidBody>(body).force, V(4.8f, 6.4f), 1e-5f));
    };

    "an attractor that is also a body is not pulled by itself"_test = [] {
        ECS::Registry world;
        world.setResource<GravitySettings>(GravitySettings{V::zero(), 1.f});
        ECS::SystemManager systems;
        systems.addSystem<GravitySystem>(world);
        const ECS::Entity moon = world.spawnEntity("moon");
        world.addComponent(moon, Transform{V(1.f, 1.f)});
        world.addComponent(moon, GravityAttractor{100.f});
        world.addComponent(moon, makeBody(10.f));

        systems.update(world, 1.0 / 60.0);

        expect(world.getComponent<RigidBody>(moon).force == V::zero());
    };

    "two attractors pull each other with opposite forces"_test = [] {
        ECS::Registry world;
        world.setResource<GravitySettings>(GravitySettings{V::zero(), 1.f});
        ECS::SystemManager systems;
        systems.addSystem<GravitySystem>(world);
        const ECS::Entity a = world.spawnEntity();
        world.addComponent(a, Transform{V(0.f, 0.f)});
        world.addComponent(a, GravityAttractor{4.f});
        world.addComponent(a, makeBody(4.f));
        const ECS::Entity b = world.spawnEntity();
        world.addComponent(b, Transform{V(2.f, 0.f)});
        world.addComponent(b, GravityAttractor{6.f});
        world.addComponent(b, makeBody(6.f));

        systems.update(world, 1.0 / 60.0);

        // 1 * 4 * 6 / 4 = 6 N each, towards the other.
        expect(near(world.getComponent<RigidBody>(a).force, V(6.f, 0.f), 1e-5f));
        expect(near(world.getComponent<RigidBody>(b).force, V(-6.f, 0.f), 1e-5f));
    };

    "the pulls of several attractors add up"_test = [] {
        ECS::Registry world;
        world.setResource<GravitySettings>(GravitySettings{V::zero(), 1.f});
        ECS::SystemManager systems;
        systems.addSystem<GravitySystem>(world);
        const ECS::Entity left = world.spawnEntity();
        world.addComponent(left, Transform{V(-1.f, 0.f)});
        world.addComponent(left, GravityAttractor{10.f});
        const ECS::Entity right = world.spawnEntity();
        world.addComponent(right, Transform{V(1.f, 0.f)});
        world.addComponent(right, GravityAttractor{10.f});
        const ECS::Entity body = world.spawnEntity();
        world.addComponent(body, Transform{V::zero()});
        world.addComponent(body, makeBody(1.f));

        systems.update(world, 1.0 / 60.0);

        expect(near(world.getComponent<RigidBody>(body).force, V::zero(), 1e-5f));
    };
}

void test_settings_change_at_runtime() {
    "a change of GravitySettings applies on the next update"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;
        systems.addSystem<GravitySystem>(world);
        const ECS::Entity body = world.spawnEntity();
        world.addComponent(body, makeBody(1.f));

        world.getResource<GravitySettings>().gravity = V(0.f, 9.81f);
        systems.update(world, 1.0 / 60.0);

        expect(world.getComponent<RigidBody>(body).force == V(0.f, 9.81f));
    };
}
}

void run_gravity_system_tests() {
    test_init_sets_default_settings();
    test_uniform_gravity();
    test_attractors();
    test_settings_change_at_runtime();
}
