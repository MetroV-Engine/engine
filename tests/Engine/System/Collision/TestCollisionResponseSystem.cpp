#include <boost/ut.hpp>

#include <algorithm>
#include <cmath>
#include <random>

#include "ECS/ECS.hpp"
#include "Engine/Common/Physic/Forces/Friction.hpp"
#include "Engine/Component/Collider.hpp"
#include "Engine/Component/SurfaceContact.hpp"
#include "Engine/System/Collision/CollisionResponseSystem.hpp"
#include "Engine/System/Collision/CollisionSystem.hpp"
#include "Engine/System/Friction/FrictionSystem.hpp"
#include "Engine/System/Gravity/GravitySystem.hpp"
#include "Engine/System/Integration/IntegrationSystem.hpp"
#include "../../Support/Approx.hpp"

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
        // 2 cm into the ground, falling at 5 m/s and moving right at 1 m/s.
        const ECS::Entity rubber = spawnBody(world, V(0.f, 0.48f), ball(0.5f, 0.8f), 1.f, V(1.f, -5.f));

        systems.update(world, Step);

        expect(near(velocityOf(world, rubber), V(1.f, 4.f), 1e-5f));     // along the ground: untouched
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
    test_resting();
    test_surface_contact();
    test_who_responds();
}
