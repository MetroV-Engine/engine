#include <boost/ut.hpp>

#include <cmath>

#include "ECS/ECS.hpp"
#include "Engine/Common/Physic/Forces/Buoyancy.hpp"
#include "Engine/Common/Shapes/Rectangle.hpp"
#include "Engine/Component/Collider.hpp"
#include "Engine/System/Buoyancy/BuoyancySystem.hpp"
#include "Engine/System/Gravity/GravitySystem.hpp"
#include "Engine/System/Integration/IntegrationSystem.hpp"
#include "../../Support/Approx.hpp"

using namespace boost::ut;
using namespace Engine;
using Component::Buoyant;
using Component::Collider;
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
    world.addComponent(box, Buoyant{Collider::Box{V(0.5f, 0.5f)}});
    return box;
}

void test_submerged_area() {
    "submergedPart: the part of the box inside the fluid and under its surface"_test = [] {
        const Shapes::Rectangle pool = Shapes::Rectangle::fromCenter(V(0.f, -2.f), V(5.f, 2.f));
        const auto area = [&pool](V center) {
            const Buoyant buoyant{Collider::Box{V(0.5f, 0.5f)}};
            return BuoyancySystem::submergedPart(buoyant.worldShape(Transform{center}), pool).area;
        };
        expect(near(area(V(0.f, -1.f)), 1.f, 1e-6f));    // under
        expect(near(area(V(0.f, 0.f)), 0.5f, 1e-6f));    // half
        expect(near(area(V(0.f, 2.f)), 0.f, 1e-6f));     // above
        expect(near(area(V(5.f, -1.f)), 0.5f, 1e-6f));   // at the edge
        expect(near(area(V(9.f, -1.f)), 0.f, 1e-6f));    // beside
        expect(near(area(V(0.f, -4.f)), 0.5f, 1e-6f));   // at the bottom
        expect(near(area(V(0.f, -6.f)), 0.f, 1e-6f));    // below
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

// Every shape floats with mass / density under the surface; the area under it is
// measured after the bodies settle (the fluid's drag damps the bobbing).
void test_every_shape() {
    "every shape floats with mass / density under the surface: box, circle, triangle, turned box"_test = [] {
        struct Case {
            Collider::Shape shape;
            float rotation;
        };
        const Case cases[] = {
            {Collider::Box{V(0.5f, 0.25f)}, 0.f},
            {Collider::Circle{0.5f}, 0.f},
            {Collider::Polygon{{{V(-0.6f, -0.4f), V(0.6f, -0.4f), V(0.f, 0.8f)}}}, 0.f},
            {Collider::Box{V(0.5f, 0.25f)}, 0.6f},    // turned, and kept turned: no Collider, no rotation
        };
        const Shapes::Rectangle pool = Shapes::Rectangle::fromCenter(V(0.f, -2.f), V(5.f, 2.f));
        for (const auto& [shape, rotation] : cases) {
            ECS::Registry world;
            world.setResource<GravitySettings>(GravitySettings{V(0.f, -10.f), 1.f});
            ECS::SystemManager systems;
            systems.addSystem<GravitySystem>(world);
            systems.addSystem<BuoyancySystem>(world);
            systems.addSystem<IntegrationSystem>(world);
            spawnPool(world, 2.f, 6.f);
            const ECS::Entity body = world.spawnEntity();
            world.addComponent(body, Transform{V(0.f, 0.3f), rotation});
            world.addComponent(body, RigidBody{0.2f});
            world.addComponent(body, Buoyant{shape});

            for (int step = 0; step < 900; ++step) {
                systems.update(world, Step);
            }

            const Physic::SubmergedPart part =
                BuoyancySystem::submergedPart(world.getComponent<Buoyant>(body).worldShape(world.getComponent<Transform>(body)), pool);
            expect(near(part.area, 0.2f / 2.f, 1e-3f)) << "shape" << shape.index() << "area" << part.area;
            expect(std::fabs(world.getComponent<RigidBody>(body).velocity.y) < 1e-3f) << "shape" << shape.index();
        }
    };

    "a circle floating at half its area has its center on the surface"_test = [] {
        ECS::Registry world;
        world.setResource<GravitySettings>(GravitySettings{V(0.f, -10.f), 1.f});
        ECS::SystemManager systems;
        systems.addSystem<GravitySystem>(world);
        systems.addSystem<BuoyancySystem>(world);
        systems.addSystem<IntegrationSystem>(world);
        spawnPool(world, 2.f, 6.f);
        const ECS::Entity ball = world.spawnEntity();
        world.addComponent(ball, Transform{V(0.f, 1.f)});
        // Mass = density * half its area: pi r² / 2 * 2.
        world.addComponent(ball, RigidBody{3.14159265f * 0.25f});
        world.addComponent(ball, Buoyant{Collider::Circle{0.5f}});

        for (int step = 0; step < 900; ++step) {
            systems.update(world, Step);
        }

        expect(near(world.getComponent<Transform>(ball).position.y, 0.f, 2e-3f));
    };

    "the push applies at the center of the part under water: off center, it turns the body"_test = [] {
        ECS::Registry world;
        world.setResource<GravitySettings>(GravitySettings{V(0.f, -10.f), 1.f});
        ECS::SystemManager systems;
        systems.addSystem<BuoyancySystem>(world);
        spawnPool(world, 2.f);
        // A 2 m by 0.5 m raft, its right half in the water: the push is under its right half.
        const ECS::Entity raft = world.spawnEntity();
        world.addComponent(raft, Transform{V(-4.f, 0.f)});
        world.addComponent(raft, RigidBody{1.f});
        world.addComponent(raft, Buoyant{Collider::Box{V(2.f, 0.25f)}});

        systems.update(world, Step);

        // The pool starts at x = -5: the raft's part in it is [-5, -2] x [-0.25, 0], 0.75 m².
        // Its centroid is at x = -3.5, 0.5 right of the raft's center: torque 0.5 * 15 N·m.
        expect(near(world.getComponent<RigidBody>(raft).force, V(0.f, 15.f), 1e-4f));
        expect(near(world.getComponent<RigidBody>(raft).torque, 7.5f, 1e-4f));
    };

    "a tilted raft rights itself: the deeper side displaces more and is pushed up harder"_test = [] {
        ECS::Registry world;
        world.setResource<GravitySettings>(GravitySettings{V(0.f, -10.f), 1.f});
        ECS::SystemManager systems;
        systems.addSystem<GravitySystem>(world);
        systems.addSystem<BuoyancySystem>(world);
        systems.addSystem<IntegrationSystem>(world);
        spawnPool(world, 2.f, 4.f);
        const ECS::Entity raft = world.spawnEntity();
        world.addComponent(raft, Transform{V(0.f, 0.f), 0.4f});
        Collider plank;
        plank.shape = Collider::Box{V(1.5f, 0.2f)};
        world.addComponent(raft, Collider{plank});
        world.addComponent(raft, RigidBody{1.2f});    // a quarter of its 1.2 m² under water
        world.addComponent(raft, Buoyant{plank.shape});

        for (int step = 0; step < 900; ++step) {
            systems.update(world, Step);
        }

        expect(std::fabs(world.getComponent<Transform>(raft).rotation) < 1e-2f) << world.getComponent<Transform>(raft).rotation;
        expect(std::fabs(world.getComponent<RigidBody>(raft).angularVelocity) < 1e-2f);
    };
}

void run_buoyancy_system_tests() {
    test_submerged_area();
    test_forces();
    test_floating();
    test_every_shape();
}
