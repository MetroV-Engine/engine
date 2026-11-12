#include <boost/ut.hpp>

#include "ECS/ECS.hpp"
#include "Engine/Common/Physic/Forces/Buoyancy.hpp"
#include "Engine/System/Buoyancy/BuoyancySystem.hpp"
#include "Engine/System/Gravity/GravitySystem.hpp"
#include "Engine/System/Integration/IntegrationSystem.hpp"
#include "../../Support/Approx.hpp"

using namespace boost::ut;
using namespace Engine;
using Component::Buoyant;
using Component::FluidVolume;
using Component::RigidBody;
using Component::Transform;
using System::BuoyancySystem;
using System::GravitySettings;
using System::GravitySystem;
using System::IntegrationSystem;
using TestSupport::near;
using V = Math::Vector2f;

namespace {
constexpr double Step = 1.0 / 60.0;

// A pool 10 m wide and 4 m deep, its surface at y = 0.
ECS::Entity spawnPool(ECS::Registry& world, float density, float drag = 0.f) {
    const ECS::Entity pool = world.spawnEntity();
    world.addComponent(pool, Transform{V(0.f, -2.f)});
    world.addComponent(pool, FluidVolume{V(5.f, 2.f), density, drag});
    return pool;
}

// A 1 m by 1 m box.
ECS::Entity spawnBox(ECS::Registry& world, V position, float mass, V velocity = V::zero()) {
    const ECS::Entity box = world.spawnEntity();
    world.addComponent(box, Transform{position});
    RigidBody body;
    body.mass = mass;
    body.velocity = velocity;
    world.addComponent(box, body);
    world.addComponent(box, Buoyant{V(0.5f, 0.5f)});
    return box;
}

void test_submerged_area() {
    "submergedArea: the part of the box inside the fluid and under its surface"_test = [] {
        const V pool(0.f, -2.f);
        const V poolHalf(5.f, 2.f);
        const V box(0.5f, 0.5f);
        expect(near(BuoyancySystem::submergedArea(V(0.f, -1.f), box, pool, poolHalf), 1.f, 1e-6f));    // under
        expect(near(BuoyancySystem::submergedArea(V(0.f, 0.f), box, pool, poolHalf), 0.5f, 1e-6f));    // half
        expect(near(BuoyancySystem::submergedArea(V(0.f, 2.f), box, pool, poolHalf), 0.f, 1e-6f));     // above
        expect(near(BuoyancySystem::submergedArea(V(5.f, -1.f), box, pool, poolHalf), 0.5f, 1e-6f));   // at the edge
        expect(near(BuoyancySystem::submergedArea(V(9.f, -1.f), box, pool, poolHalf), 0.f, 1e-6f));    // beside
        expect(near(BuoyancySystem::submergedArea(V(0.f, -4.f), box, pool, poolHalf), 0.5f, 1e-6f));   // at the bottom
        expect(near(BuoyancySystem::submergedArea(V(0.f, -6.f), box, pool, poolHalf), 0.f, 1e-6f));    // below
    };
}

void test_forces() {
    "a body under the surface is pushed up by the weight of the fluid it displaces"_test = [] {
        ECS::Registry world;
        world.setResource<GravitySettings>(GravitySettings{V(0.f, -10.f), 1.f});
        ECS::SystemManager systems;
        systems.addSystem<BuoyancySystem>(world);
        spawnPool(world, 2.f);
        const ECS::Entity under = spawnBox(world, V(0.f, -1.f), 1.f);
        const ECS::Entity half = spawnBox(world, V(2.f, 0.f), 1.f);
        const ECS::Entity above = spawnBox(world, V(0.f, 3.f), 1.f);

        systems.update(world, Step);

        expect(near(world.getComponent<RigidBody>(under).force, V(0.f, 20.f), 1e-5f));
        expect(near(world.getComponent<RigidBody>(half).force,
                    Physic::buoyancyForce(2.f, 0.5f, V(0.f, -10.f)), 1e-5f));
        expect(world.getComponent<RigidBody>(above).force == V::zero());
    };

    "the fluid's drag slows a body in proportion to its area under the surface"_test = [] {
        ECS::Registry world;
        world.setResource<GravitySettings>(GravitySettings{V::zero(), 1.f});
        ECS::SystemManager systems;
        systems.addSystem<BuoyancySystem>(world);
        spawnPool(world, 0.f, 3.f);
        const ECS::Entity under = spawnBox(world, V(0.f, -1.f), 1.f, V(2.f, 0.f));
        const ECS::Entity half = spawnBox(world, V(2.f, 0.f), 1.f, V(2.f, 0.f));

        systems.update(world, Step);

        const float dt = static_cast<float>(Step);
        expect(near(world.getComponent<RigidBody>(under).force, Physic::linearDragForStep(V(2.f, 0.f), 3.f, 1.f, dt), 1e-5f));
        expect(near(world.getComponent<RigidBody>(half).force, Physic::linearDragForStep(V(2.f, 0.f), 1.5f, 1.f, dt), 1e-5f));
    };

    "without GravitySettings, the default gravity; a body without Buoyant does not float"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;
        systems.addSystem<BuoyancySystem>(world);
        spawnPool(world, 1.f);
        const ECS::Entity box = spawnBox(world, V(0.f, -1.f), 1.f);
        const ECS::Entity plain = world.spawnEntity();
        world.addComponent(plain, Transform{V(1.f, -1.f)});
        world.addComponent(plain, RigidBody{});

        systems.update(world, Step);

        expect(near(world.getComponent<RigidBody>(box).force, V(0.f, Physic::StandardGravity<float>), 1e-5f));
        expect(world.getComponent<RigidBody>(plain).force == V::zero());
        expect(systems.getSystem<BuoyancySystem>()->name() == "BuoyancySystem");
    };

    "two pools add up where they overlap"_test = [] {
        ECS::Registry world;
        world.setResource<GravitySettings>(GravitySettings{V(0.f, -10.f), 1.f});
        ECS::SystemManager systems;
        systems.addSystem<BuoyancySystem>(world);
        spawnPool(world, 1.f);
        spawnPool(world, 1.f);
        const ECS::Entity box = spawnBox(world, V(0.f, -1.f), 1.f);

        systems.update(world, Step);

        expect(near(world.getComponent<RigidBody>(box).force, V(0.f, 20.f), 1e-5f));
    };
}

void test_floating() {
    "a body lighter than the fluid floats with mass / density under the surface"_test = [] {
        ECS::Registry world;
        world.setResource<GravitySettings>(GravitySettings{V(0.f, -10.f), 1.f});
        ECS::SystemManager systems;
        systems.addSystem<GravitySystem>(world);
        systems.addSystem<BuoyancySystem>(world);
        systems.addSystem<IntegrationSystem>(world);
        spawnPool(world, 2.f, 4.f);
        // 0.6 kg of a 1 m² box in a fluid of 2 kg/m²: 0.3 m² under the surface.
        const ECS::Entity box = spawnBox(world, V(0.f, 1.f), 0.6f);

        for (int i = 0; i < 60 * 20; i++) {
            systems.update(world, Step);
        }

        const RigidBody& body = world.getComponent<RigidBody>(box);
        const float bottom = world.getComponent<Transform>(box).position.y - 0.5f;
        expect(near(bottom, -0.3f, 1e-3f));
        expect(near(body.velocity.y, 0.f, 1e-3f));
    };

    "a body heavier than the fluid sinks"_test = [] {
        ECS::Registry world;
        world.setResource<GravitySettings>(GravitySettings{V(0.f, -10.f), 1.f});
        ECS::SystemManager systems;
        systems.addSystem<GravitySystem>(world);
        systems.addSystem<BuoyancySystem>(world);
        systems.addSystem<IntegrationSystem>(world);
        spawnPool(world, 2.f, 4.f);
        const ECS::Entity box = spawnBox(world, V(0.f, -1.f), 5.f);

        for (int i = 0; i < 60; i++) {
            systems.update(world, Step);
        }

        expect(world.getComponent<Transform>(box).position.y < -1.5f);
        expect(world.getComponent<RigidBody>(box).velocity.y < 0.f);
    };
}
}

void run_buoyancy_system_tests() {
    test_submerged_area();
    test_forces();
    test_floating();
}
