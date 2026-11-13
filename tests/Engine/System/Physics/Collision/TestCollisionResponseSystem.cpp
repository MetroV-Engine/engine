#include <boost/ut.hpp>

#include <algorithm>
#include <cmath>
#include <random>

#include "ECS/ECS.hpp"
#include "Engine/Common/Physic/Forces/Friction.hpp"
#include "Engine/Common/Physic/Rotation.hpp"
#include "Engine/Component/Physics/Collider.hpp"
#include "Engine/Component/Physics/SurfaceContact.hpp"
#include "Engine/System/Physics/Collision/CollisionResponseSystem.hpp"
#include "Engine/System/Physics/Collision/CollisionSystem.hpp"
#include "Engine/System/Physics/Friction/FrictionSystem.hpp"
#include "Engine/System/Physics/Gravity/GravitySystem.hpp"
#include "Engine/System/Physics/Integration/IntegrationSystem.hpp"
#include "../../../Support/Approx.hpp"

using namespace boost::ut;
using namespace Engine;
using Component::Collider;
using Component::RigidBody;
using Component::SurfaceContact;
using Component::Transform;
using System::CollisionResponseSystem;
using System::CollisionSettings;
using System::CollisionSystem;
using TestSupport::near;
using V = Math::Vector2f;

namespace {
constexpr double Step = 1.0 / 60.0;
constexpr float Gravity = 9.81f;

Collider box(V halfSize, float restitution = 0.3f) {
    Collider collider;
    collider.shape = Collider::Box{halfSize};
    collider.restitution = restitution;
    return collider;
}

Collider ball(float radius, float restitution) {
    Collider collider;
    collider.shape = Collider::Circle{radius};
    collider.restitution = restitution;
    return collider;
}

Collider stopping(Collider collider) {
    collider.response = Collider::Response::Stop;
    return collider;
}

ECS::Entity spawnStatic(ECS::Registry& world, V position, const Collider& collider) {
    const ECS::Entity entity = world.spawnEntity();
    world.addComponent(entity, Transform{position});
    world.addComponent(entity, Collider{collider});
    return entity;
}

ECS::Entity spawnBody(ECS::Registry& world, V position, const Collider& collider, float mass = 1.f, V velocity = {}) {
    const ECS::Entity entity = spawnStatic(world, position, collider);
    world.addComponent(entity, RigidBody{mass, 1.f, velocity, {}});
    return entity;
}

/** @brief The ground: its top at y = 0, 200 m wide. */
ECS::Entity spawnGround(ECS::Registry& world, const Collider& collider = box(V(100.f, 1.f))) {
    return spawnStatic(world, V(0.f, -1.f), collider);
}

/** @brief Only the collision: detection then response, no gravity, no motion. */
void addCollision(ECS::Registry& world, ECS::SystemManager& systems) {
    systems.addSystem<CollisionSystem>(world);
    systems.addSystem<CollisionResponseSystem>(world);
}

/** @brief A whole physics step, in the documented order. */
void addPhysics(ECS::Registry& world, ECS::SystemManager& systems) {
    systems.addSystem<Engine::System::GravitySystem>(world);
    systems.addSystem<Engine::System::FrictionSystem>(world);
    systems.addSystem<Engine::System::IntegrationSystem>(world);
    addCollision(world, systems);
}

V& velocityOf(ECS::Registry& world, ECS::Entity entity) { return world.getComponent<RigidBody>(entity).velocity; }
V& positionOf(ECS::Registry& world, ECS::Entity entity) { return world.getComponent<Transform>(entity).position; }

void test_one_impact() {
    "a body hitting the ground bounces back at e times its speed, and leaves the ground"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;
        addCollision(world, systems);
        spawnGround(world);
        // Just touching the ground (so its contact point is at its radius), falling at 5 m/s
        // and moving right at 1 m/s.
        const ECS::Entity rubber = spawnBody(world, V(0.f, 0.5f - 1e-5f), ball(0.5f, 0.8f), 1.f, V(1.f, -5.f));

        systems.update(world, Step);

        // Along the normal: back at e = 0.8 of 5 m/s. Along the ground, friction during the
        // impact grips the ball: it leaves rolling, at 2/3 of its speed for a disc
        // (I = m r² / 2), spinning at -v / r.
        expect(near(velocityOf(world, rubber), V(2.f / 3.f, 4.f), 1e-4f));
        expect(near(world.getComponent<RigidBody>(rubber).angularVelocity, -4.f / 3.f, 1e-3f));
        expect(near(positionOf(world, rubber).y, 0.5f, 1e-6f));           // out of the ground, touching it
    };

    "the bouncier collider sets the restitution, whichever is on the ground"_test = [] {
        for (const bool bouncyGround : {false, true}) {
            ECS::Registry world;
            ECS::SystemManager systems;
            addCollision(world, systems);
            spawnGround(world, box(V(100.f, 1.f), bouncyGround ? 0.9f : 0.1f));
            const ECS::Entity crate = spawnBody(world, V(0.f, 0.49f), box(V(0.5f, 0.5f), bouncyGround ? 0.1f : 0.9f), 1.f, V(0.f, -10.f));

            systems.update(world, Step);

            expect(near(velocityOf(world, crate).y, 9.f, 1e-5f));
        }
    };

    "an impact slower than restingSpeed does not bounce"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;
        addCollision(world, systems);
        world.getResource<CollisionSettings>().restingSpeed = 0.5f;
        spawnGround(world);
        const ECS::Entity slow = spawnBody(world, V(0.f, 0.49f), ball(0.5f, 1.f), 1.f, V(0.f, -0.4f));
        const ECS::Entity fast = spawnBody(world, V(5.f, 0.49f), ball(0.5f, 1.f), 1.f, V(0.f, -0.6f));

        systems.update(world, Step);

        expect(velocityOf(world, slow).y == 0.f);
        expect(near(velocityOf(world, fast).y, 0.6f, 1e-6f));
    };

    "bodies already moving apart are separated but keep their velocity"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;
        addCollision(world, systems);
        spawnGround(world);
        const ECS::Entity rising = spawnBody(world, V(0.f, 0.4f), ball(0.5f, 0.8f), 1.f, V(0.f, 2.f));

        systems.update(world, Step);

        expect(velocityOf(world, rising) == V(0.f, 2.f));
        expect(near(positionOf(world, rising).y, 0.5f, 1e-6f));
    };
}

void test_stop() {
    "Stop: a character landing stops along the normal and keeps its run"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;
        addCollision(world, systems);
        spawnGround(world, box(V(100.f, 1.f), 0.9f));      // even on a very bouncy floor
        const ECS::Entity player = spawnBody(world, V(0.f, 0.88f), stopping(box(V(0.4f, 0.9f))), 70.f, V(6.f, -12.f));

        systems.update(world, Step);

        expect(near(velocityOf(world, player), V(6.f, 0.f), 1e-5f));
        expect(near(positionOf(world, player).y, 0.9f, 1e-6f));
    };

    "Stop: a character hitting a wall stops against it and keeps falling"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;
        addCollision(world, systems);
        spawnStatic(world, V(2.f, 0.f), box(V(0.5f, 5.f), 0.9f));
        const ECS::Entity player = spawnBody(world, V(1.15f, 0.f), stopping(box(V(0.4f, 0.9f))), 70.f, V(5.f, -3.f));

        systems.update(world, Step);

        expect(near(velocityOf(world, player), V(0.f, -3.f), 1e-5f));
        expect(near(positionOf(world, player).x, 1.1f, 1e-6f));
    };

    "Stop: a character running into a crate pushes it, both leave at the speed that conserves momentum"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;
        addCollision(world, systems);
        const ECS::Entity player = spawnBody(world, V(0.f, 0.f), stopping(box(V(0.4f, 0.9f))), 70.f, V(5.f, 0.f));
        const ECS::Entity crate = spawnBody(world, V(0.85f, 0.f), box(V(0.5f, 0.5f), 0.9f), 5.f);

        systems.update(world, Step);

        const float together = 70.f * 5.f / 75.f;
        expect(near(velocityOf(world, player).x, together, 1e-4f));
        expect(near(velocityOf(world, crate).x, together, 1e-4f));
    };

    "the ball next to the character still bounces on the same ground"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;
        addCollision(world, systems);
        spawnGround(world);
        const ECS::Entity player = spawnBody(world, V(0.f, 0.88f), stopping(box(V(0.4f, 0.9f))), 70.f, V(0.f, -5.f));
        const ECS::Entity rubber = spawnBody(world, V(3.f, 0.48f), ball(0.5f, 0.8f), 0.5f, V(0.f, -5.f));

        systems.update(world, Step);

        expect(velocityOf(world, player).y == 0.f);
        expect(near(velocityOf(world, rubber).y, 4.f, 1e-5f));
    };
}

void test_proportions() {
    "elastic impact of equal masses: they swap velocities, like billiard balls"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;
        addCollision(world, systems);
        const ECS::Entity left = spawnBody(world, V(0.f, 0.f), ball(0.5f, 1.f), 1.f, V(3.f, 0.f));
        const ECS::Entity right = spawnBody(world, V(0.95f, 0.f), ball(0.5f, 1.f), 1.f, V(-1.f, 0.f));

        systems.update(world, Step);

        expect(near(velocityOf(world, left), V(-1.f, 0.f), 1e-5f));
        expect(near(velocityOf(world, right), V(3.f, 0.f), 1e-5f));
    };

    "the overlap is shared by inverse mass: the heavy body moves less, the center of mass stays"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;
        addCollision(world, systems);
        const ECS::Entity heavy = spawnBody(world, V(0.f, 0.f), box(V(0.5f, 0.5f)), 3.f);
        const ECS::Entity light = spawnBody(world, V(0.6f, 0.f), box(V(0.5f, 0.5f)), 1.f);

        systems.update(world, Step);

        // 0.4 m of overlap: the light body moves 3 / 4 of it, the heavy one 1 / 4.
        expect(near(positionOf(world, heavy).x, -0.1f, 1e-6f));
        expect(near(positionOf(world, light).x, 0.9f, 1e-6f));
        expect(near(3.f * positionOf(world, heavy).x + positionOf(world, light).x, 0.6f, 1e-5f));
    };

    "random impacts: momentum is conserved and kinetic energy never grows"_test = [] {
        std::mt19937 random(7);
        const auto uniform = [&random](float low, float high) { return std::uniform_real_distribution<float>(low, high)(random); };
        for (int test = 0; test < 500; ++test) {
            ECS::Registry world;
            ECS::SystemManager systems;
            addCollision(world, systems);
            const float massA = uniform(0.1f, 100.f);
            const float massB = uniform(0.1f, 100.f);
            const V velocityA(uniform(-20.f, 20.f), uniform(-20.f, 20.f));
            const V velocityB(uniform(-20.f, 20.f), uniform(-20.f, 20.f));
            Collider colliderA = test % 2 ? ball(0.5f, uniform(0.f, 1.f)) : box(V(0.5f, 0.5f), uniform(0.f, 1.f));
            Collider colliderB = test % 3 ? ball(0.5f, uniform(0.f, 1.f)) : box(V(0.5f, 0.5f), uniform(0.f, 1.f));
            if (test % 5 == 0) {
                colliderA.response = Collider::Response::Stop;
            }
            const ECS::Entity a = spawnBody(world, V(0.f, 0.f), colliderA, massA, velocityA);
            const ECS::Entity b = spawnBody(world, V(uniform(-0.9f, 0.9f), uniform(-0.9f, 0.9f)), colliderB, massB, velocityB);

            systems.update(world, Step);

            const V momentumBefore = velocityA * massA + velocityB * massB;
            const V momentumAfter = velocityOf(world, a) * massA + velocityOf(world, b) * massB;
            const float energyBefore = massA * velocityA.lengthSquared() + massB * velocityB.lengthSquared();
            const float energyAfter = massA * velocityOf(world, a).lengthSquared() + massB * velocityOf(world, b).lengthSquared();
            expect(near(momentumAfter, momentumBefore, 1e-3f * (1.f + momentumBefore.length()))) << "test" << test;
            expect(energyAfter <= energyBefore * (1.f + 1e-5f) + 1e-4f) << "test" << test;
        }
    };
}

void test_static_friction_at_impact() {
    // A 100 kg crate resting on the ground (mu static 0.5): over one step its
    // static friction holds a velocity change of 0.5 * 9.81 / 60 = 0.082 m/s.
    const auto restingCrate = [](ECS::Registry& world, ECS::SystemManager& systems) {
        addPhysics(world, systems);
        spawnGround(world);
        const ECS::Entity crate = spawnBody(world, V(0.f, 0.5f), box(V(0.5f, 0.5f), 0.f), 100.f);
        systems.update(world, Step);    // settles, gets its SurfaceContact
        return crate;
    };

    "a resting crate is not budged by an impact its static friction holds: the other body stops against it"_test = [&] {
        ECS::Registry world;
        ECS::SystemManager systems;
        const ECS::Entity crate = restingCrate(world, systems);
        // 1 kg at 5 m/s, already touching: shared, the crate would get 5 / 101 = 0.05 m/s.
        const ECS::Entity pebble = spawnBody(world, V(-0.7f, 0.5f), ball(0.2f, 0.f), 1.f, V(5.f, 0.f));

        systems.update(world, Step);

        expect(velocityOf(world, crate).x == 0.f);
        expect(near(positionOf(world, crate).x, 0.f, 1e-6f));
        expect(near(velocityOf(world, pebble).x, 0.f, 1e-5f)) << "stopped as against a wall";
    };

    "an impact stronger than its static friction moves the resting crate, sharing the momentum"_test = [&] {
        ECS::Registry world;
        ECS::SystemManager systems;
        const ECS::Entity crate = restingCrate(world, systems);
        // 20 kg at 5 m/s: shared, the crate would get 100 / 120 = 0.83 m/s.
        const ECS::Entity heavy = spawnBody(world, V(-0.9f, 0.5f), box(V(0.4f, 0.4f), 0.f), 20.f, V(5.f, 0.f));

        systems.update(world, Step);

        const float together = 20.f * 5.f / 120.f;
        expect(near(velocityOf(world, crate).x, together, 0.06f));
        expect(near(velocityOf(world, heavy).x, together, 0.06f));
    };

    "a crate already sliding is not held: kinetic friction is FrictionSystem's"_test = [&] {
        ECS::Registry world;
        ECS::SystemManager systems;
        const ECS::Entity crate = restingCrate(world, systems);
        velocityOf(world, crate) = V(1.f, 0.f);
        const ECS::Entity pebble = spawnBody(world, V(-0.7f, 0.5f), ball(0.2f, 0.f), 1.f, V(6.f, 0.f));

        systems.update(world, Step);

        // Friction leaves it 1 - 0.049 m/s; the pebble then adds 5 / 101 m/s.
        // Held, it would stay at 0.951 and stop the pebble dead.
        const float shared = (6.f + 100.f * (1.f - 0.3f * 9.81f / 60.f)) / 101.f;
        expect(near(velocityOf(world, crate).x, shared, 1e-3f));
        expect(near(velocityOf(world, pebble).x, shared, 1e-3f));
    };

    "an impact from below lifts the crate: static friction does not hold it down"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;
        addPhysics(world, systems);
        // The crate rests on two tiles, over the gap between them.
        spawnStatic(world, V(-1.f, -0.5f), box(V(0.7f, 0.5f)));
        spawnStatic(world, V(1.f, -0.5f), box(V(0.7f, 0.5f)));
        const ECS::Entity crate = spawnBody(world, V(0.f, 0.5f), box(V(0.5f, 0.5f), 0.f), 100.f);
        systems.update(world, Step);
        // A pebble shot up through the gap, at a slight angle, into the crate's underside.
        const ECS::Entity pebble = spawnBody(world, V(0.f, -0.2f), ball(0.15f, 0.f), 1.f, V(0.2f, 6.f));
        world.getComponent<RigidBody>(pebble).gravityScale = 0.f;

        systems.update(world, Step);

        expect(velocityOf(world, crate).y > 0.f);
    };
}

/**
 * @brief A slope rising to the right at angle degrees, 20 m wide, its
 *        surface the line y = x tan(angle) through (0, 0); and a 1 m crate
 *        resting on it at x = 0 (its lowest corner, bottom-right, on it).
 */
struct SlopeScene {
    ECS::Registry world;
    ECS::SystemManager systems;
    ECS::Entity crate;
    float angle;

    /** @brief level: the crate set upright on its corner; otherwise lying flat on the slope. */
    explicit SlopeScene(float degrees, bool level = false, const Collider& crateCollider = box(V(0.5f, 0.5f), 0.f))
        : angle(degrees * 3.14159265f / 180.f) {
        addPhysics(world, systems);
        const float rise = std::tan(angle);
        Collider slope;
        slope.shape = Collider::Polygon::slope(V(10.f, 10.f * rise));
        spawnStatic(world, V(0.f, 0.f), slope);
        if (level) {
            crate = spawnBody(world, V(0.f, rise * 0.5f + 0.5f), crateCollider, 10.f);
        } else {
            // Its bottom face on the surface: the center half a meter along the slope's normal.
            const V normal(-std::sin(angle), std::cos(angle));
            crate = spawnBody(world, normal * 0.5f, crateCollider, 10.f);
            world.getComponent<Transform>(crate).rotation = angle;
        }
    }

    /** @brief How far the crate went down the slope, along it. */
    float slid(const V& start) {
        const V moved = positionOf(world, crate) - start;
        return -(moved.x * std::cos(angle) + moved.y * std::sin(angle));
    }
};

void test_slopes() {
    "a crate lying on a slope whose tangent is below its static friction stays (20°: 0.36 < 0.5)"_test = [] {
        SlopeScene scene(20.f);
        const V start = positionOf(scene.world, scene.crate);

        for (int step = 0; step < 120; ++step) {
            scene.systems.update(scene.world, Step);
        }

        expect(std::fabs(scene.slid(start)) < 1e-3f) << scene.slid(start);
        expect(scene.world.hasComponents<SurfaceContact>(scene.crate));
        const V normal = scene.world.getComponent<SurfaceContact>(scene.crate).normal;
        expect(near(normal, V(-std::sin(scene.angle), std::cos(scene.angle)), 1e-5f)) << "the slope's normal";
        expect(near(scene.world.getComponent<Transform>(scene.crate).rotation, scene.angle, 1e-4f)) << "still flat on it";
    };

    "a crate set upright on its corner tips over onto the slope and stays there, flat on it"_test = [] {
        // Rubber: kinetic friction 0.8 above tan 20°, so the tipping cannot start a slide.
        Collider rubber = box(V(0.5f, 0.5f), 0.f);
        rubber.staticFriction = 1.f;
        rubber.kineticFriction = 0.8f;
        SlopeScene scene(20.f, true, rubber);

        for (int step = 0; step < 180; ++step) {
            scene.systems.update(scene.world, Step);
        }

        expect(near(scene.world.getComponent<Transform>(scene.crate).rotation, scene.angle, 1e-3f))
            << scene.world.getComponent<Transform>(scene.crate).rotation;
        expect(velocityOf(scene.world, scene.crate).length() < 1e-3f);
        expect(std::fabs(scene.world.getComponent<RigidBody>(scene.crate).angularVelocity) < 1e-3f);
    };

    "on a steeper slope it slides down at g (sin - muKinetic cos): 4.86 m/s² at 45°"_test = [] {
        SlopeScene scene(45.f);
        const V start = positionOf(scene.world, scene.crate);

        for (int step = 0; step < 60; ++step) {
            scene.systems.update(scene.world, Step);
        }

        const float acceleration = Gravity * (std::sin(scene.angle) - 0.3f * std::cos(scene.angle));
        expect(near(scene.slid(start), acceleration / 2.f, 0.05f * acceleration / 2.f)) << scene.slid(start);
        expect(near(velocityOf(scene.world, scene.crate).length(), acceleration, 0.03f * acceleration));
        // Sliding along the slope, not bouncing off it.
        const V along(std::cos(scene.angle), std::sin(scene.angle));
        expect(std::fabs(velocityOf(scene.world, scene.crate).cross(along)) < 1e-3f);
    };

    "a ball dropped on a slope bounces off along the slope's normal, not straight up"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;
        addCollision(world, systems);
        Collider slope;
        slope.shape = Collider::Polygon::slope(V(5.f, 5.f));    // 45°, surface y = x
        spawnStatic(world, V(0.f, 0.f), slope);
        // Falling at 4 m/s, just touching the slope.
        const float diagonal = std::sqrt(0.5f);
        const ECS::Entity rubber =
            spawnBody(world, V(-0.25f * diagonal, 0.25f * diagonal - 1e-5f), ball(0.25f, 1.f), 1.f, V(0.f, -4.f));

        systems.update(world, Step);

        // e = 1: the part along the normal, -4 cos 45°, is turned around. Friction during
        // the impact grips the ball: along the slope it keeps 2/3 of -4 sin 45° and spins.
        const V normal(-diagonal, diagonal);
        const V along(diagonal, diagonal);
        const float fall = 4.f * diagonal;
        expect(near(velocityOf(world, rubber), normal * fall - along * (fall * 2.f / 3.f), 1e-3f));
        // Rolling: its contact point does not slide on the slope.
        const RigidBody& body = world.getComponent<RigidBody>(rubber);
        const V contactPoint = body.velocity + (normal * -0.25f).perpendicular() * body.angularVelocity;
        expect(std::fabs(contactPoint.dot(along)) < 1e-3f);
    };
}

void test_new_contact_friction() {
    "a body landing gently is slowed by friction from its first contact, Response::Stop is not"_test = [] {
        for (const bool stop : {false, true}) {
            ECS::Registry world;
            ECS::SystemManager systems;
            addCollision(world, systems);
            spawnGround(world);
            Collider crate = box(V(0.5f, 0.5f), 0.f);
            crate.response = stop ? Collider::Response::Stop : Collider::Response::Bounce;
            // Sliding at 3 m/s, settling at 0.2 m/s: below restingSpeed, no bounce.
            const ECS::Entity body = spawnBody(world, V(0.f, 0.49f), crate, 1.f, V(3.f, -0.2f));

            systems.update(world, Step);

            // Normal impulse 0.2 N·s; kinetic friction 0.3 of it, 0.06 m/s on 1 kg.
            expect(near(velocityOf(world, body), V(stop ? 3.f : 2.94f, 0.f), 1e-5f)) << "Stop:" << stop;
        }
    };
}

void test_rotation() {
    "an off-center hit spins a bar, a centered one does not; momentum and angular momentum are kept"_test = [] {
        for (const float hitAt : {0.f, 0.8f}) {
            ECS::Registry world;
            ECS::SystemManager systems;
            addCollision(world, systems);
            const ECS::Entity bar = spawnBody(world, V(0.f, 0.f), box(V(1.f, 0.1f), 1.f), 1.f);
            const ECS::Entity pebble = spawnBody(world, V(hitAt, -0.19f), ball(0.1f, 1.f), 0.5f, V(0.f, 3.f));
            const auto momentum = [&] { return velocityOf(world, bar) + velocityOf(world, pebble) * 0.5f; };
            // About the origin: I w + r x m v of each body.
            const float barInertia = Physic::boxInertia(1.f, V(1.f, 0.1f));
            // With the positions before the step: the impulses act there, the position
            // correction then moves the bodies.
            const V barAt = positionOf(world, bar);
            const V pebbleAt = positionOf(world, pebble);
            // The pebble spins too: friction where the bar's turning surface rubs it.
            const float pebbleInertia = Physic::discInertia(0.5f, 0.1f);
            const auto angular = [&] {
                return barInertia * world.getComponent<RigidBody>(bar).angularVelocity
                     + pebbleInertia * world.getComponent<RigidBody>(pebble).angularVelocity
                     + barAt.cross(velocityOf(world, bar))
                     + pebbleAt.cross(velocityOf(world, pebble) * 0.5f);
            };
            const V momentumBefore = momentum();
            const float angularBefore = angular();

            systems.update(world, Step);

            const float spin = world.getComponent<RigidBody>(bar).angularVelocity;
            expect(hitAt == 0.f ? std::fabs(spin) < 1e-5f : spin > 0.5f) << "hit at" << hitAt << "spin" << spin;
            expect(near(momentum(), momentumBefore, 1e-4f));
            expect(near(angular(), angularBefore, 1e-4f)) << angular() << angularBefore;
        }
    };

    "a crate dropped on its corner falls over and comes to rest flat on the ground"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;
        addPhysics(world, systems);
        spawnGround(world);
        const ECS::Entity crate = spawnBody(world, V(0.f, 1.5f), box(V(0.5f, 0.5f)), 10.f);
        world.getComponent<Transform>(crate).rotation = 0.5f;

        for (int step = 0; step < 240; ++step) {
            systems.update(world, Step);
        }

        const float rotation = world.getComponent<Transform>(crate).rotation;
        const float quarter = 3.14159265f / 2.f;
        const float offFlat = std::fabs(rotation - quarter * std::round(rotation / quarter));
        expect(offFlat < 1e-3f) << "rotation" << rotation;
        expect(near(positionOf(world, crate).y, 0.5f, 1e-3f));
        expect(velocityOf(world, crate).length() < 1e-3f);
        expect(std::fabs(world.getComponent<RigidBody>(crate).angularVelocity) < 1e-3f);
    };

    "a ball rolls down a slope at 2/3 g sin(angle), without slipping"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;
        addPhysics(world, systems);
        const float angle = 30.f * 3.14159265f / 180.f;
        Collider slope;
        slope.shape = Collider::Polygon::slope(V(10.f, 10.f * std::tan(angle)));
        spawnStatic(world, V(0.f, 0.f), slope);
        const V normal(-std::sin(angle), std::cos(angle));
        const ECS::Entity rubber = spawnBody(world, normal * 0.25f, ball(0.25f, 0.f), 1.f);

        for (int step = 0; step < 60; ++step) {
            systems.update(world, Step);
        }

        const RigidBody& body = world.getComponent<RigidBody>(rubber);
        // A disc: a = g sin / (1 + I / (m r²)) = 2/3 g sin.
        expect(near(body.velocity.length(), 2.f / 3.f * Gravity * std::sin(angle), 0.05f)) << body.velocity.length();
        const V contactPoint = body.velocity + (normal * -0.25f).perpendicular() * body.angularVelocity;
        expect(contactPoint.length() < 0.02f) << "slipping at" << contactPoint.length();
    };

    "a ball thrown sliding without spin ends up rolling at 2/3 of its speed"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;
        addPhysics(world, systems);
        spawnGround(world);
        const ECS::Entity rubber = spawnBody(world, V(0.f, 0.25f), ball(0.25f, 0.f), 1.f);
        systems.update(world, Step);
        velocityOf(world, rubber) = V(3.f, 0.f);

        for (int step = 0; step < 120; ++step) {
            systems.update(world, Step);
        }

        // Friction turns sliding into spin until the contact point stops: v = v0 / (1 + I / (m r²)).
        expect(near(velocityOf(world, rubber).x, 2.f, 0.02f)) << velocityOf(world, rubber).x;
        expect(near(world.getComponent<RigidBody>(rubber).angularVelocity, -2.f / 0.25f, 0.1f));
    };

    "random impacts of turned shapes: momentum and angular momentum are conserved, energy never grows"_test = [] {
        std::mt19937 random(5);
        const auto uniform = [&random](float low, float high) { return std::uniform_real_distribution<float>(low, high)(random); };
        int failures = 0;
        int spun = 0;
        for (int test = 0; test < 500; ++test) {
            ECS::Registry world;
            ECS::SystemManager systems;
            addCollision(world, systems);
            Collider forms[3];
            forms[0] = box(V(uniform(0.2f, 1.f), uniform(0.2f, 1.f)), uniform(0.f, 1.f));
            forms[1] = ball(uniform(0.2f, 1.f), uniform(0.f, 1.f));
            forms[2].shape = Collider::Polygon::slope(V(uniform(0.3f, 1.f), uniform(0.3f, 1.f)));
            const Collider& formA = forms[test % 3];
            const Collider& formB = forms[(test / 3) % 3];
            const float massA = uniform(0.5f, 20.f);
            const float massB = uniform(0.5f, 20.f);
            const ECS::Entity a = spawnBody(world, V(0.f, 0.f), formA, massA, V(uniform(-5.f, 5.f), uniform(-5.f, 5.f)));
            const ECS::Entity b = spawnBody(world, V(uniform(-0.6f, 0.6f), uniform(-0.6f, 0.6f)), formB, massB,
                                            V(uniform(-5.f, 5.f), uniform(-5.f, 5.f)));
            for (const ECS::Entity entity : {a, b}) {
                world.getComponent<Transform>(entity).rotation = uniform(-3.f, 3.f);
                world.getComponent<RigidBody>(entity).angularVelocity = uniform(-4.f, 4.f);
            }
            const float inertiaA = formA.momentOfInertia(massA);
            const float inertiaB = formB.momentOfInertia(massB);
            const auto momentum = [&] { return velocityOf(world, a) * massA + velocityOf(world, b) * massB; };
            const auto angular = [&] {
                return inertiaA * world.getComponent<RigidBody>(a).angularVelocity + positionOf(world, a).cross(velocityOf(world, a) * massA)
                     + inertiaB * world.getComponent<RigidBody>(b).angularVelocity + positionOf(world, b).cross(velocityOf(world, b) * massB);
            };
            const auto energy = [&] {
                const float spinA = world.getComponent<RigidBody>(a).angularVelocity;
                const float spinB = world.getComponent<RigidBody>(b).angularVelocity;
                return massA * velocityOf(world, a).lengthSquared() + inertiaA * spinA * spinA
                     + massB * velocityOf(world, b).lengthSquared() + inertiaB * spinB * spinB;
            };
            const V momentumBefore = momentum();
            const float energyBefore = energy();
            // Angular momentum about a fixed point is kept by the impulses; the position
            // correction moves the bodies, so measure it with the positions before.
            const V positionA = positionOf(world, a);
            const V positionB = positionOf(world, b);
            const float angularBefore = angular();
            const float spinBefore = world.getComponent<RigidBody>(a).angularVelocity;

            systems.update(world, Step);

            const float angularAfter = inertiaA * world.getComponent<RigidBody>(a).angularVelocity + positionA.cross(velocityOf(world, a) * massA)
                                     + inertiaB * world.getComponent<RigidBody>(b).angularVelocity + positionB.cross(velocityOf(world, b) * massB);
            const float scale = 1.f + momentumBefore.length() + std::fabs(angularBefore) + energyBefore;
            const bool kept = near(momentum(), momentumBefore, 1e-3f * scale) && near(angularAfter, angularBefore, 1e-3f * scale)
                && energy() <= energyBefore * (1.f + 1e-4f) + 1e-3f;
            failures += kept ? 0 : 1;
            spun += world.getComponent<RigidBody>(a).angularVelocity != spinBefore ? 1 : 0;
        }
        expect(failures == 0_i);
        expect(spun > 100_i) << "the impacts must turn the bodies";
    };
}

void test_resting() {
    "a crate dropped on the ground settles and stays still, with no jitter"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;
        addPhysics(world, systems);
        spawnGround(world);
        const ECS::Entity crate = spawnBody(world, V(0.f, 2.f), box(V(0.5f, 0.5f)), 10.f);

        float highestAfterSettling = -1.f;
        float lowestAfterSettling = 10.f;
        for (int step = 0; step < 300; ++step) {
            systems.update(world, Step);
            if (step >= 120) {
                highestAfterSettling = std::max(highestAfterSettling, positionOf(world, crate).y);
                lowestAfterSettling = std::min(lowestAfterSettling, positionOf(world, crate).y);
                expect(world.hasComponents<SurfaceContact>(crate)) << "step" << step;
            }
        }

        expect(near(highestAfterSettling, 0.5f, 1e-4f));
        expect(near(lowestAfterSettling, 0.5f, 1e-4f));
        expect(velocityOf(world, crate).length() < 1e-3f);
    };

    "a crate across two tiles is pushed out once, not once per tile"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;
        addPhysics(world, systems);
        spawnStatic(world, V(-0.5f, -0.5f), box(V(0.5f, 0.5f)));
        spawnStatic(world, V(0.5f, -0.5f), box(V(0.5f, 0.5f)));
        const ECS::Entity crate = spawnBody(world, V(0.f, 0.5f), box(V(0.5f, 0.5f)), 10.f);

        float highest = 0.f;
        for (int step = 0; step < 120; ++step) {
            systems.update(world, Step);
            highest = std::max(highest, positionOf(world, crate).y);
        }

        expect(near(highest, 0.5f, 1e-4f));
        expect(near(positionOf(world, crate).y, 0.5f, 1e-4f));
    };

    "a dropped ball bounces lower each time, about e² of the height, then comes to rest"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;
        addPhysics(world, systems);
        spawnGround(world);
        const float e = 0.8f;
        const float dropHeight = 2.f;    // of the ball's bottom
        const ECS::Entity rubber = spawnBody(world, V(0.f, dropHeight + 0.25f), ball(0.25f, e), 0.5f);

        float apex = 0.f;
        bool firstApexFound = false;
        bool wasRising = false;
        for (int step = 0; step < 900; ++step) {
            systems.update(world, Step);
            const bool rising = velocityOf(world, rubber).y > 0.f;
            if (wasRising && !rising && !firstApexFound) {
                apex = positionOf(world, rubber).y - 0.25f;
                firstApexFound = true;
            }
            wasRising = rising;
        }

        expect(fatal(firstApexFound));
        expect(near(apex, e * e * dropHeight, 0.05f * dropHeight)) << "apex" << apex;
        expect(near(positionOf(world, rubber).y, 0.25f, 1e-3f));
        expect(velocityOf(world, rubber).length() < 1e-3f);
    };
}

void test_surface_contact() {
    "landing adds a SurfaceContact with the combined friction, leaving removes it"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;
        addCollision(world, systems);
        Collider ice = box(V(100.f, 1.f));
        ice.staticFriction = 0.05f;
        ice.kineticFriction = 0.02f;
        spawnGround(world, ice);
        Collider rubber = box(V(0.5f, 0.5f));
        rubber.staticFriction = 1.25f;
        rubber.kineticFriction = 0.8f;
        const ECS::Entity crate = spawnBody(world, V(0.f, 0.49f), rubber, 1.f, V(0.f, -1.f));

        systems.update(world, Step);

        expect(fatal(world.hasComponents<SurfaceContact>(crate)));
        const SurfaceContact& contact = world.getComponent<SurfaceContact>(crate);
        expect(near(contact.normal, V(0.f, 1.f), 1e-6f));
        expect(near(contact.staticFriction, 0.25f, 1e-6f));
        expect(near(contact.kineticFriction, std::sqrt(0.02f * 0.8f), 1e-6f));
        expect(contact.surfaceVelocity == V::zero());

        positionOf(world, crate).y = 3.f;
        systems.update(world, Step);

        expect(!world.hasComponents<SurfaceContact>(crate));
    };

    "the surface normal points to the body, whichever of the pair comes first"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;
        addCollision(world, systems);
        // The tile starts right of the crate: the crate is the first of the
        // pair, and the contact normal points down, from it to the tile.
        spawnStatic(world, V(0.3f, -0.5f), box(V(0.5f, 0.5f)));
        const ECS::Entity crate = spawnBody(world, V(0.f, 0.49f), box(V(0.5f, 0.5f)), 1.f, V(0.f, -1.f));

        systems.update(world, Step);

        expect(fatal(world.getResource<System::Contacts>().pairs.size() == 1_u));
        expect(world.getResource<System::Contacts>().pairs[0].first == crate);
        expect(fatal(world.hasComponents<SurfaceContact>(crate)));
        expect(near(world.getComponent<SurfaceContact>(crate).normal, V(0.f, 1.f), 1e-6f));
        expect(near(positionOf(world, crate).y, 0.5f, 1e-6f));
        expect(near(velocityOf(world, crate).y, 0.3f, 1e-6f));    // bounces up, at e = 0.3 of 1 m/s
    };

    "a body touching the ground and a wall keeps the ground as its surface"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;
        addCollision(world, systems);
        spawnGround(world);
        spawnStatic(world, V(-1.f, 2.f), box(V(0.5f, 2.f)));
        const ECS::Entity crate = spawnBody(world, V(0.f, 0.5f), box(V(0.5f, 0.5f)));

        systems.update(world, Step);

        expect(fatal(world.hasComponents<SurfaceContact>(crate)));
        expect(near(world.getComponent<SurfaceContact>(crate).normal, V(0.f, 1.f), 1e-6f));
    };

    "end to end: a crate sliding on the ground stops after v² / (2 mu g)"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;
        addPhysics(world, systems);
        spawnGround(world);
        const ECS::Entity crate = spawnBody(world, V(0.f, 0.5f), box(V(0.5f, 0.5f)), 10.f);
        systems.update(world, Step);    // settles, gets its SurfaceContact
        velocityOf(world, crate) = V(4.f, 0.f);
        const float start = positionOf(world, crate).x;

        for (int step = 0; step < 180; ++step) {
            systems.update(world, Step);
        }

        const float mu = 0.3f;    // both colliders have the default kinetic friction
        expect(near(positionOf(world, crate).x - start, 16.f / (2.f * mu * Gravity), 0.1f));
        expect(velocityOf(world, crate).length() < 1e-4f);
    };

    "a crate on a sliding platform is carried along by friction"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;
        addPhysics(world, systems);
        const ECS::Entity platform = spawnBody(world, V(0.f, -0.25f), box(V(3.f, 0.25f)), 0.f, V(2.f, 0.f));
        const ECS::Entity crate = spawnBody(world, V(0.f, 0.5f), box(V(0.5f, 0.5f)), 10.f);

        for (int step = 0; step < 60; ++step) {
            systems.update(world, Step);
        }

        expect(near(velocityOf(world, crate).x, 2.f, 1e-3f));
        expect(near(positionOf(world, crate).y, 0.5f, 1e-3f));
        expect(velocityOf(world, platform) == V(2.f, 0.f));    // kinematic: never pushed
    };

    "on a turning platform, the surface moves at the contact point's velocity, spin included"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;
        addCollision(world, systems);
        // A kinematic bar turning at 2 rad/s around its center; a crate resting 1.5 m right of it.
        const ECS::Entity bar = spawnBody(world, V(0.f, -0.25f), box(V(3.f, 0.25f)), 0.f);
        world.getComponent<RigidBody>(bar).angularVelocity = 2.f;
        const ECS::Entity crate = spawnBody(world, V(1.5f, 0.49f), box(V(0.5f, 0.5f)), 10.f);

        systems.update(world, Step);

        expect(world.hasComponents<SurfaceContact>(crate) >> fatal);
        const SurfaceContact& contact = world.getComponent<SurfaceContact>(crate);
        // v = w x r, r from the bar's center to the contact, about (1.5, 0.25): (-2 * 0.25, 2 * 1.5).
        expect(near(contact.surfaceVelocity.y, 3.f, 0.05f)) << contact.surfaceVelocity.y;
        expect(near(contact.surfaceVelocity.x, -0.5f, 0.05f)) << contact.surfaceVelocity.x;
    };

    "a crate slammed down while sliding on the ground loses speed to friction during the impact"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;
        addCollision(world, systems);
        spawnGround(world);
        const ECS::Entity crate = spawnBody(world, V(0.f, 0.49f), box(V(0.5f, 0.5f), 0.f), 10.f);
        systems.update(world, Step);  // the contact exists: the next impact is not a new contact
        velocityOf(world, crate) = V(3.f, -4.f);

        systems.update(world, Step);

        // The normal impulse is at least 4 m (more with the ground's restitution 0.3, and the crate
        // tipping on its front corner): kinetic friction 0.3 takes at least 1.2 m/s of the slide.
        // Without friction during impacts, the slide would be kept whole. It never reverses.
        expect(velocityOf(world, crate).x <= 3.f - 0.3f * 4.f) << velocityOf(world, crate).x;
        expect(velocityOf(world, crate).x >= 0.f) << velocityOf(world, crate).x;
    };

    "a rising platform lifts the crate on it"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;
        addPhysics(world, systems);
        const ECS::Entity platform = spawnBody(world, V(0.f, -0.25f), box(V(3.f, 0.25f)), 0.f, V(0.f, 1.f));
        const ECS::Entity crate = spawnBody(world, V(0.f, 0.5f), box(V(0.5f, 0.5f)), 10.f);

        for (int step = 0; step < 60; ++step) {
            systems.update(world, Step);
        }

        const float platformTop = positionOf(world, platform).y + 0.25f;
        expect(near(platformTop, 1.f, 1e-3f));
        expect(near(positionOf(world, crate).y, platformTop + 0.5f, 2e-2f));
    };
}

void test_surface_choice() {
    "across two flat floors, the floor under the center is the surface, whatever else is in the scene"_test = [] {
        bool always = true;
        int compared = 0;
        for (const float centerX : {-0.1f, 0.f, 0.1f}) {
            for (const bool iceFirst : {true, false}) {
                for (int others = 0; others <= 60; others += 3) {
                    ECS::Registry world;
                    ECS::SystemManager systems;
                    addCollision(world, systems);
                    Collider ice = box(V(1.f, 1.f));
                    ice.staticFriction = 0.1f;
                    Collider rubber = box(V(1.f, 1.f));
                    rubber.staticFriction = 1.f;
                    // Ice left of x = 0, rubber right, both tops at y = 0: the crate across
                    // them touches two surfaces facing up exactly as much.
                    const auto spawnIce = [&] { return spawnStatic(world, V(-1.f, -1.f), ice); };
                    const auto spawnRubber = [&] { return spawnStatic(world, V(1.f, -1.f), rubber); };
                    const ECS::Entity first = iceFirst ? spawnIce() : spawnRubber();
                    iceFirst ? spawnRubber() : spawnIce();
                    const ECS::Entity crate = spawnBody(world, V(centerX, 0.5f), box(V(0.5f, 0.5f)), 1.f);
                    // Unrelated crates far away, each one more surface for the response to sort.
                    spawnStatic(world, V(200.f, -1.f), box(V(100.f, 1.f)));
                    for (int index = 0; index < others; ++index) {
                        spawnBody(world, V(110.f + 2.f * static_cast<float>(index), 0.5f), box(V(0.5f, 0.5f)), 1.f);
                    }

                    systems.update(world, Step);

                    // Under the center; exactly on the joint, the floor spawned first.
                    const float floor = centerX < 0.f ? 0.1f
                        : centerX > 0.f              ? 1.f
                                                     : world.getComponent<Collider>(first).staticFriction;
                    const bool found = world.hasComponents<SurfaceContact>(crate)
                        && world.getComponent<SurfaceContact>(crate).staticFriction == Physic::combinedFriction(0.5f, floor);
                    always = always && found;
                    expect(found) << "center" << centerX << "ice first" << iceFirst << "others" << others;
                    ++compared;
                }
            }
        }
        expect(compared == 126_i);
        expect(always);
    };
}

void test_who_responds() {
    "static and kinematic colliders are never moved by the response"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;
        addCollision(world, systems);
        const ECS::Entity wall = spawnStatic(world, V(0.f, 0.f), box(V(1.f, 1.f)));
        const ECS::Entity platform = spawnBody(world, V(1.5f, 0.f), box(V(1.f, 1.f)), 0.f, V(-1.f, 0.f));

        systems.update(world, Step);

        expect(positionOf(world, wall) == V(0.f, 0.f));
        expect(positionOf(world, platform) == V(1.5f, 0.f));
        expect(velocityOf(world, platform) == V(-1.f, 0.f));
        expect(!world.hasComponents<SurfaceContact>(platform));
    };

    "init sets default CollisionSettings, and keeps the ones already set"_test = [] {
        ECS::Registry world;
        world.setResource<CollisionSettings>().restingSpeed = 2.f;
        ECS::SystemManager systems;
        systems.addSystem<CollisionResponseSystem>(world);
        expect(world.getResource<CollisionSettings>().restingSpeed == 2.f);
        expect(world.hasResource<System::Contacts>());

        ECS::Registry other;
        ECS::SystemManager otherSystems;
        otherSystems.addSystem<CollisionResponseSystem>(other);
        expect(other.getResource<CollisionSettings>().restingSpeed == CollisionSettings{}.restingSpeed);
        expect(otherSystems.getSystem<CollisionResponseSystem>()->name() == "CollisionResponseSystem");
    };
}
}

void run_collision_response_system_tests() {
    test_one_impact();
    test_stop();
    test_proportions();
    test_static_friction_at_impact();
    test_slopes();
    test_new_contact_friction();
    test_rotation();
    test_resting();
    test_surface_contact();
    test_surface_choice();
    test_who_responds();
}
