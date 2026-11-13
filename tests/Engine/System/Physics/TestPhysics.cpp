#include <boost/ut.hpp>

#include <algorithm>
#include <cmath>

#include "ECS/ECS.hpp"
#include "Engine/System/Physics/Physics.hpp"
#include "../../Support/Approx.hpp"

using namespace boost::ut;
using namespace Engine;
using Component::Buoyant;
using Component::CharacterController;
using Component::CharacterInput;
using Component::Collider;
using Component::FluidVolume;
using Component::RigidBody;
using Component::Transform;
using TestSupport::near;
using V = Math::Vector2f;

namespace {
ECS::Entity spawn(ECS::Registry& world, V position, Collider::Shape shape, float mass = -1.f) {
    const ECS::Entity entity = world.spawnEntity();
    world.addComponent(entity, Transform{position});
    Collider collider;
    collider.shape = shape;
    world.addComponent(entity, Collider{collider});
    if (mass >= 0.f) {
        world.addComponent(entity, RigidBody{mass});
    }
    return entity;
}

void test_install() {
    "install registers the ten physics systems, once each"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;
        Physics::install(world, systems);

        expect(systems.size() == 10_u);
        expect(systems.getSystem<System::GravitySystem>() != nullptr);
        expect(systems.getSystem<System::SpringSystem>() != nullptr);
        expect(systems.getSystem<System::RadialFieldSystem>() != nullptr);
        expect(systems.getSystem<System::DragSystem>() != nullptr);
        expect(systems.getSystem<System::BuoyancySystem>() != nullptr);
        expect(systems.getSystem<System::CharacterControllerSystem>() != nullptr);
        expect(systems.getSystem<System::FrictionSystem>() != nullptr);
        expect(systems.getSystem<System::IntegrationSystem>() != nullptr);
        expect(systems.getSystem<System::CollisionSystem>() != nullptr);
        expect(systems.getSystem<System::CollisionResponseSystem>() != nullptr);
        expect(world.hasResource<System::GravitySettings>());
        expect(world.hasResource<System::Contacts>());
        expect(world.hasResource<System::CollisionSettings>());
    };

    "a whole scene runs on it: a crate settles, a ball rolls, a character jumps its height, a raft floats"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;
        Physics::install(world, systems);
        spawn(world, V(0.f, -1.f), Collider::Box{V(50.f, 1.f)});                      // ground, top at y = 0
        const ECS::Entity crate = spawn(world, V(-10.f, 2.f), Collider::Box{V(0.5f, 0.5f)}, 20.f);
        const ECS::Entity rubber = spawn(world, V(10.f, 0.25f), Collider::Circle{0.25f}, 1.f);
        world.getComponent<RigidBody>(rubber).velocity = V(3.f, 0.f);
        const ECS::Entity hero = spawn(world, V(0.f, 0.9f), Collider::Box{V(0.4f, 0.9f)}, 70.f);
        world.getComponent<Collider>(hero).response = Collider::Response::Stop;
        world.addComponent(hero, CharacterController{});
        world.addComponent(hero, CharacterInput{});
        const ECS::Entity pool = world.spawnEntity();
        world.addComponent(pool, Transform{V(30.f, 2.f)});
        world.addComponent(pool, FluidVolume{V(5.f, 2.f), 2.f, 12.f});    // a thick fluid: the raft settles fast
        const ECS::Entity raft = spawn(world, V(30.f, 4.5f), Collider::Box{V(1.f, 0.25f)}, 0.5f);
        world.addComponent(raft, Buoyant{Collider::Box{V(1.f, 0.25f)}});

        for (int step = 0; step < 60; ++step) {
            systems.update(world, Physics::Step);
        }
        world.getComponent<CharacterInput>(hero).jump = true;
        float highest = 0.f;
        for (int step = 0; step < 180; ++step) {
            systems.update(world, Physics::Step);
            highest = std::max(highest, world.getComponent<Transform>(hero).position.y - 0.9f);
        }

        expect(near(world.getComponent<Transform>(crate).position.y, 0.5f, 1e-3f));
        expect(world.getComponent<RigidBody>(crate).velocity.length() < 1e-3f);
        expect(near(world.getComponent<RigidBody>(rubber).velocity.x, 2.f, 0.05f)) << "rolling at 2/3 of 3 m/s";
        expect(highest >= 2.5f - 1e-3f && highest <= 2.5f + 1e-3f) << highest;
        expect(world.getComponent<Transform>(hero).rotation == 0.f);
        // 0.5 kg on a fluid of 2 kg/m²: 0.25 m² under the surface (y = 4), of a 2 m wide raft.
        expect(near(world.getComponent<Transform>(raft).position.y, 4.f + 0.25f - 0.125f, 5e-3f))
            << world.getComponent<Transform>(raft).position.y << "rotation" << world.getComponent<Transform>(raft).rotation;
    };
}
}

void run_physics_tests() {
    test_install();
}
