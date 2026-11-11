#include <boost/ut.hpp>

#include "ECS/ECS.hpp"
#include "Engine/System/Spring/SpringSystem.hpp"
#include "../../Support/Approx.hpp"

using namespace boost::ut;
using namespace Engine;
using Component::RigidBody;
using Component::Spring;
using Component::Transform;
using System::SpringSystem;
using TestSupport::near;
using V = Math::Vector2f;

namespace {
Spring makeSpring(ECS::Entity body, ECS::Entity other, V anchor, float restLength, float stiffness, float damping = 0.f) {
    Spring spring;
    spring.body = body;
    spring.other = other;
    spring.anchor = anchor;
    spring.restLength = restLength;
    spring.stiffness = stiffness;
    spring.damping = damping;
    return spring;
}

// Springs only, no gravity: the forces one step adds, before integration.
struct Scene {
    ECS::Registry world;
    ECS::SystemManager systems;

    Scene() { systems.addSystem<SpringSystem>(world); }

    ECS::Entity body(V position, float mass = 1.f, V velocity = V::zero(), V force = V::zero()) {
        const ECS::Entity entity = world.spawnEntity();
        world.addComponent(entity, Transform{position});
        RigidBody body;
        body.mass = mass;
        body.velocity = velocity;
        body.force = force;
        world.addComponent(entity, body);
        return entity;
    }

    // An end that holds a spring without moving: a hook in a wall.
    ECS::Entity hook(V position) {
        const ECS::Entity entity = world.spawnEntity("hook");
        world.addComponent(entity, Transform{position});
        return entity;
    }

    // Springs live on entities of their own.
    void link(const Spring& spring) { world.addComponent(world.spawnEntity("spring"), spring); }

    void step() { systems.update(world, 1.0 / 60.0); }
    V force(ECS::Entity entity) { return world.getComponent<RigidBody>(entity).force; }
};

void test_spring_to_a_point() {
    "a stretched spring pulls the body towards the fixed point"_test = [] {
        Scene scene;
        const ECS::Entity body = scene.body(V(3.f, 0.f));
        scene.link(makeSpring(body, {}, V::zero(), 1.f, 10.f));

        scene.step();

        expect(near(scene.force(body), V(-20.f, 0.f), 1e-5f));
    };

    "a compressed spring pushes the body away"_test = [] {
        Scene scene;
        const ECS::Entity body = scene.body(V(0.5f, 0.f));
        scene.link(makeSpring(body, {}, V::zero(), 1.f, 10.f));

        scene.step();

        expect(near(scene.force(body), V(5.f, 0.f), 1e-5f));
    };

    "damping acts on the speed along the spring only"_test = [] {
        Scene scene;
        const ECS::Entity body = scene.body(V(2.f, 0.f), 1.f, V(3.f, 7.f));
        scene.link(makeSpring(body, {}, V::zero(), 2.f, 10.f, 4.f));

        scene.step();

        // At rest length: only damping, -4 * 3 along X; the sideways 7 m/s is not slowed.
        expect(near(scene.force(body), V(-12.f, 0.f), 1e-5f));
    };

    "spring forces add to the forces already received"_test = [] {
        Scene scene;
        const ECS::Entity body = scene.body(V(3.f, 0.f), 1.f, V::zero(), V(1.f, 1.f));
        scene.link(makeSpring(body, {}, V::zero(), 1.f, 10.f));

        scene.step();

        expect(near(scene.force(body), V(-19.f, 1.f), 1e-5f));
    };
}

void test_spring_between_entities() {
    "a spring between two bodies pulls both, with opposite forces"_test = [] {
        Scene scene;
        const ECS::Entity a = scene.body(V(0.f, 0.f), 1.f);
        const ECS::Entity b = scene.body(V(4.f, 0.f), 3.f);
        scene.link(makeSpring(a, b, V::zero(), 2.f, 5.f));

        scene.step();

        expect(near(scene.force(a), V(10.f, 0.f), 1e-5f));
        expect(near(scene.force(b), V(-10.f, 0.f), 1e-5f));
    };

    "damping between two bodies uses their relative velocity"_test = [] {
        Scene scene;
        const ECS::Entity togetherA = scene.body(V(0.f, 0.f), 1.f, V(2.f, 0.f));
        const ECS::Entity togetherB = scene.body(V(2.f, 0.f), 1.f, V(2.f, 0.f));
        scene.link(makeSpring(togetherA, togetherB, V::zero(), 2.f, 10.f, 3.f));
        const ECS::Entity closingA = scene.body(V(0.f, 5.f), 1.f, V(1.f, 0.f));
        const ECS::Entity closingB = scene.body(V(2.f, 5.f), 1.f, V(-1.f, 0.f));
        scene.link(makeSpring(closingA, closingB, V::zero(), 2.f, 10.f, 3.f));

        scene.step();

        // Moving together: nothing to damp.
        expect(near(scene.force(togetherA), V::zero(), 1e-5f));
        // Closing in at 2 m/s: damping pushes them apart with 3 * 2 = 6 N each.
        expect(near(scene.force(closingA), V(-6.f, 0.f), 1e-5f));
        expect(near(scene.force(closingB), V(6.f, 0.f), 1e-5f));
    };

    "an end without RigidBody holds the spring without moving"_test = [] {
        Scene scene;
        const ECS::Entity body = scene.body(V(3.f, 0.f));
        const ECS::Entity hook = scene.hook(V::zero());
        scene.link(makeSpring(body, hook, V(100.f, 100.f), 1.f, 10.f));

        scene.step();

        // The hook is the other end: `anchor` is ignored.
        expect(near(scene.force(body), V(-20.f, 0.f), 1e-5f));
        expect(!scene.world.hasComponents<RigidBody>(hook));
    };

    "a spring can also start from the end that does not move"_test = [] {
        Scene scene;
        const ECS::Entity hook = scene.hook(V::zero());
        const ECS::Entity body = scene.body(V(3.f, 0.f));
        scene.link(makeSpring(hook, body, V::zero(), 1.f, 10.f));

        scene.step();

        // The hook gets nothing; the body, at the other end, is pulled back the same way.
        expect(near(scene.force(body), V(-20.f, 0.f), 1e-5f));
    };

    "the springs on a body add up"_test = [] {
        Scene scene;
        const ECS::Entity body = scene.body(V(0.5f, 0.f));
        scene.link(makeSpring(body, {}, V(-2.f, 0.f), 1.f, 10.f));
        scene.link(makeSpring(body, {}, V(2.f, 0.f), 1.f, 10.f));

        scene.step();

        // Left: stretched by 1.5, 15 N to the left; right: stretched by 0.5, 5 N to the right.
        expect(near(scene.force(body), V(-10.f, 0.f), 1e-5f));
    };
}

void test_broken_springs() {
    "a spring whose other end was killed does nothing"_test = [] {
        Scene scene;
        const ECS::Entity body = scene.body(V(3.f, 0.f));
        const ECS::Entity other = scene.body(V(0.f, 0.f));
        scene.link(makeSpring(body, other, V::zero(), 1.f, 10.f));
        scene.world.killEntity(other);

        scene.step();

        expect(scene.force(body) == V::zero());
    };

    "a spring whose body has no Transform does nothing"_test = [] {
        Scene scene;
        const ECS::Entity floating = scene.world.spawnEntity();
        scene.world.addComponent(floating, RigidBody{});
        scene.link(makeSpring(floating, {}, V(5.f, 0.f), 1.f, 10.f));

        scene.step();

        expect(scene.force(floating) == V::zero());
    };
}
}

void run_spring_system_tests() {
    test_spring_to_a_point();
    test_spring_between_entities();
    test_broken_springs();
}
