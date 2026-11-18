#include <boost/ut.hpp>

#include "ECS/ECS.hpp"
#include "Engine/Common/Physic/Forces/Radial.hpp"
#include "Engine/System/Physics/Radial/RadialFieldSystem.hpp"
#include "../../../Support/Approx.hpp"

using namespace boost::ut;
using namespace Engine;
using Component::RadialField;
using Component::RigidBody;
using Component::Transform;
using System::RadialFieldSystem;
using TestSupport::near;
using V = Math::Vector2f;

namespace {
ECS::Entity spawnBody(ECS::Registry& world, V position) {
    const ECS::Entity body = world.spawnEntity();
    world.addComponent(body, Transform{position});
    world.addComponent(body, RigidBody{});
    return body;
}

ECS::Entity spawnField(ECS::Registry& world, V position, float strength, float radius) {
    const ECS::Entity field = world.spawnEntity();
    world.addComponent(field, Transform{position});
    world.addComponent(field, RadialField{strength, radius});
    return field;
}

void test_push() {
    "a field pushes the bodies in its radius away, with Physic::radialPush"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;
        systems.addSystem<RadialFieldSystem>(world);
        spawnField(world, V(1.f, 1.f), 10.f, 4.f);
        const ECS::Entity right = spawnBody(world, V(3.f, 1.f));
        const ECS::Entity below = spawnBody(world, V(1.f, 0.f));

        systems.update(world, 1.0 / 60.0);

        // Half way to the radius: half the strength.
        expect(near(world.getComponent<RigidBody>(right).force, V(5.f, 0.f), 1e-5f));
        expect(near(world.getComponent<RigidBody>(below).force,
                    Physic::radialPush(V(1.f, 0.f), V(1.f, 1.f), 10.f, 4.f), 1e-6f));
        expect(world.getComponent<RigidBody>(below).force.y < 0.f);
    };

    "a negative strength pulls; bodies at or beyond the radius get nothing"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;
        systems.addSystem<RadialFieldSystem>(world);
        spawnField(world, V::zero(), -10.f, 2.f);
        const ECS::Entity inside = spawnBody(world, V(1.f, 0.f));
        const ECS::Entity atRadius = spawnBody(world, V(0.f, 2.f));
        const ECS::Entity far = spawnBody(world, V(10.f, 0.f));

        systems.update(world, 1.0 / 60.0);

        expect(near(world.getComponent<RigidBody>(inside).force, V(-5.f, 0.f), 1e-5f));
        expect(world.getComponent<RigidBody>(atRadius).force == V::zero());
        expect(world.getComponent<RigidBody>(far).force == V::zero());
    };

    "fields add up, to the forces already received; a field does not push its own body"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;
        systems.addSystem<RadialFieldSystem>(world);
        spawnField(world, V(-1.f, 0.f), 4.f, 2.f);
        spawnField(world, V(0.f, -1.f), 4.f, 2.f);
        const ECS::Entity body = spawnBody(world, V::zero());
        world.getComponent<RigidBody>(body).force = V(1.f, 1.f);
        // A moving repulsor: a field that is also a body.
        const ECS::Entity repulsor = spawnField(world, V(5.f, 5.f), 100.f, 3.f);
        world.addComponent(repulsor, RigidBody{});

        systems.update(world, 1.0 / 60.0);

        expect(near(world.getComponent<RigidBody>(body).force, V(3.f, 3.f), 1e-5f));
        expect(world.getComponent<RigidBody>(repulsor).force == V::zero());
    };

    "a body without a Transform is not pushed"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;
        systems.addSystem<RadialFieldSystem>(world);
        spawnField(world, V::zero(), 10.f, 5.f);
        const ECS::Entity body = world.spawnEntity();
        world.addComponent(body, RigidBody{});

        systems.update(world, 1.0 / 60.0);

        expect(world.getComponent<RigidBody>(body).force == V::zero());
        expect(systems.getSystem<RadialFieldSystem>()->name() == "RadialFieldSystem");
    };
}
}

void run_radial_field_system_tests() {
    test_push();
}
