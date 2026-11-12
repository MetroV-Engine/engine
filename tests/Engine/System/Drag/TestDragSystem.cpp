#include <boost/ut.hpp>

#include <cmath>

#include "ECS/ECS.hpp"
#include "Engine/Common/Physic/Forces/Drag.hpp"
#include "Engine/System/Drag/DragSystem.hpp"
#include "Engine/System/Gravity/GravitySystem.hpp"
#include "Engine/System/Integration/IntegrationSystem.hpp"
#include "../../Support/Approx.hpp"

using namespace boost::ut;
using namespace Engine;
using Component::Drag;
using Component::RigidBody;
using Component::Transform;
using System::DragSystem;
using System::GravitySettings;
using System::GravitySystem;
using System::IntegrationSystem;
using TestSupport::near;
using V = Math::Vector2f;

namespace {
constexpr double Step = 1.0 / 60.0;

ECS::Entity spawnBody(ECS::Registry& world, float mass, V velocity, Drag drag) {
    const ECS::Entity body = world.spawnEntity();
    world.addComponent(body, Transform{});
    RigidBody rigidBody;
    rigidBody.mass = mass;
    rigidBody.velocity = velocity;
    world.addComponent(body, rigidBody);
    world.addComponent(body, drag);
    return body;
}

void test_forces() {
    "the drag of a step: linear and quadratic, from the *ForStep formulas"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;
        systems.addSystem<DragSystem>(world);
        const ECS::Entity linear = spawnBody(world, 2.f, V(4.f, 0.f), Drag{0.5f, 0.f, {}});
        const ECS::Entity quadratic = spawnBody(world, 2.f, V(0.f, -3.f), Drag{0.f, 0.1f, {}});
        const ECS::Entity both = spawnBody(world, 2.f, V(1.f, 1.f), Drag{0.5f, 0.1f, {}});

        systems.update(world, Step);

        const float dt = static_cast<float>(Step);
        expect(near(world.getComponent<RigidBody>(linear).force,
                    Physic::linearDragForStep(V(4.f, 0.f), 0.5f, 2.f, dt), 1e-6f));
        expect(near(world.getComponent<RigidBody>(quadratic).force, V(0.f, 0.9f), 1e-5f));
        // The quadratic drag acts on what the linear one leaves of the velocity.
        const V linearForce = Physic::linearDragForStep(V(1.f, 1.f), 0.5f, 2.f, dt);
        const V remaining = V(1.f, 1.f) + linearForce * (dt / 2.f);
        expect(near(world.getComponent<RigidBody>(both).force,
                    linearForce + Physic::quadraticDragForStep(remaining, 0.1f, 2.f, dt), 1e-6f));
    };

    "the drag opposes the velocity relative to the flow: a wind pushes a body at rest"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;
        systems.addSystem<DragSystem>(world);
        const ECS::Entity still = spawnBody(world, 1.f, V::zero(), Drag{1.f, 0.f, V(5.f, 0.f)});
        const ECS::Entity withWind = spawnBody(world, 1.f, V(5.f, 0.f), Drag{1.f, 1.f, V(5.f, 0.f)});

        systems.update(world, Step);

        expect(world.getComponent<RigidBody>(still).force.x > 0.f);
        expect(world.getComponent<RigidBody>(withWind).force == V::zero());
    };

    "no coefficient, a static body or a body without Drag: no force"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;
        systems.addSystem<DragSystem>(world);
        const ECS::Entity none = spawnBody(world, 1.f, V(3.f, 0.f), Drag{});
        const ECS::Entity kinematic = spawnBody(world, 0.f, V(3.f, 0.f), Drag{1.f, 1.f, {}});
        const ECS::Entity plain = world.spawnEntity();
        RigidBody moving;
        moving.velocity = V(3.f, 0.f);
        world.addComponent(plain, moving);

        systems.update(world, Step);

        expect(world.getComponent<RigidBody>(none).force == V::zero());
        expect(world.getComponent<RigidBody>(kinematic).force == V::zero());
        expect(world.getComponent<RigidBody>(plain).force == V::zero());
        expect(systems.getSystem<DragSystem>()->name() == "DragSystem");
    };
}

void test_motion() {
    "linear drag slows a body exactly as v0 e^(-c t / m), whatever the step"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;
        systems.addSystem<DragSystem>(world);
        systems.addSystem<IntegrationSystem>(world);
        const ECS::Entity body = spawnBody(world, 2.f, V(10.f, 0.f), Drag{1.f, 0.f, {}});

        for (int i = 0; i < 60; i++) {
            systems.update(world, Step);
        }

        expect(near(world.getComponent<RigidBody>(body).velocity.x, 10.f * std::exp(-0.5f), 1e-4f));
    };

    "a very strong drag stops a body without sending it backwards"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;
        systems.addSystem<DragSystem>(world);
        systems.addSystem<IntegrationSystem>(world);
        const ECS::Entity body = spawnBody(world, 0.1f, V(5.f, 0.f), Drag{1000.f, 1000.f, {}});

        for (int i = 0; i < 10; i++) {
            systems.update(world, Step);
            // At most a float rounding below 0: it stops, it does not come back.
            expect(world.getComponent<RigidBody>(body).velocity.x >= -1e-5f) << world.getComponent<RigidBody>(body).velocity.x;
        }
    };

    "a falling body with quadratic drag reaches sqrt(m g / c)"_test = [] {
        ECS::Registry world;
        world.setResource<GravitySettings>(GravitySettings{V(0.f, -10.f), 1.f});
        ECS::SystemManager systems;
        systems.addSystem<GravitySystem>(world);
        systems.addSystem<DragSystem>(world);
        systems.addSystem<IntegrationSystem>(world);
        const ECS::Entity body = spawnBody(world, 1.f, V::zero(), Drag{0.f, 0.1f, {}});

        for (int i = 0; i < 60 * 20; i++) {
            systems.update(world, Step);
        }

        expect(near(world.getComponent<RigidBody>(body).velocity.y, -10.f, 1e-2f));
    };
}
}

void run_drag_system_tests() {
    test_forces();
    test_motion();
}
