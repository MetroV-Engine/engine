#include <boost/ut.hpp>

#include <cmath>

#include "ECS/ECS.hpp"
#include "Engine/Common/Physic/Forces/Surface.hpp"
#include "Engine/System/Physics/Friction/FrictionSystem.hpp"
#include "Engine/System/Physics/Gravity/GravitySystem.hpp"
#include "../../../Support/Approx.hpp"

using namespace boost::ut;
using namespace Engine;
using Component::RigidBody;
using Component::SurfaceContact;
using System::FrictionSystem;
using System::GravitySettings;
using System::GravitySystem;
using TestSupport::near;
using V = Math::Vector2f;

namespace {
constexpr double Dt = 1.0 / 60.0;

RigidBody makeBody(float mass, V velocity = V::zero(), V force = V::zero()) {
    RigidBody body;
    body.mass = mass;
    body.velocity = velocity;
    body.force = force;
    return body;
}

SurfaceContact makeContact(V normal, float staticFriction, float kineticFriction, V surfaceVelocity = V::zero()) {
    SurfaceContact contact;
    contact.normal = normal;
    contact.staticFriction = staticFriction;
    contact.kineticFriction = kineticFriction;
    contact.surfaceVelocity = surfaceVelocity;
    return contact;
}

// g = 10 with Y up, then friction: the forces a body gets in one step, before integration.
struct Scene {
    ECS::Registry world;
    ECS::SystemManager systems;

    explicit Scene(V gravity = V(0.f, -10.f)) {
        world.setResource<GravitySettings>(GravitySettings{gravity, 1.f});
        systems.addSystem<GravitySystem>(world);
        systems.addSystem<FrictionSystem>(world);
    }

    ECS::Entity body(const RigidBody& body, const SurfaceContact* contact = nullptr) {
        const ECS::Entity entity = world.spawnEntity();
        world.addComponent(entity, body);
        if (contact) {
            world.addComponent(entity, *contact);
        }
        return entity;
    }

    V forceAfterStep(ECS::Entity entity) {
        systems.update(world, Dt);
        return world.getComponent<RigidBody>(entity).force;
    }
};

const V Up{0.f, 1.f};

void test_flat_ground() {
    // 1 kg on flat ground under g = 10: N = 10, so static holds up to 5 N, kinetic is 3 N.
    const SurfaceContact ground = makeContact(Up, 0.5f, 0.3f);

    "a body without SurfaceContact gets no friction"_test = [] {
        Scene scene;
        const ECS::Entity body = scene.body(makeBody(1.f, V(4.f, 0.f), V(3.f, 0.f)));

        expect(scene.forceAfterStep(body) == V(3.f, -10.f));
    };

    "static friction holds a resting body against a push within muStatic * N"_test = [=] {
        Scene scene;
        const ECS::Entity body = scene.body(makeBody(1.f, V::zero(), V(3.f, 0.f)), &ground);

        expect(near(scene.forceAfterStep(body), V(0.f, -10.f), 1e-5f));
    };

    "a push above muStatic * N starts the body sliding, against muKinetic * N"_test = [=] {
        Scene scene;
        const ECS::Entity body = scene.body(makeBody(1.f, V::zero(), V(8.f, 0.f)), &ground);

        expect(near(scene.forceAfterStep(body), V(5.f, -10.f), 1e-4f));
    };

    "kinetic friction opposes the sliding velocity"_test = [=] {
        Scene scene;
        const ECS::Entity body = scene.body(makeBody(1.f, V(-5.f, 0.f)), &ground);

        expect(near(scene.forceAfterStep(body), V(3.f, -10.f), 1e-4f));
    };

    "friction grows with the mass pressing on the surface"_test = [=] {
        Scene scene;
        const ECS::Entity heavy = scene.body(makeBody(4.f, V(5.f, 0.f)), &ground);

        expect(near(scene.forceAfterStep(heavy), V(-12.f, -40.f), 1e-4f));
    };

    "a body pulled away from the surface gets no friction"_test = [=] {
        Scene scene(V::zero());
        const ECS::Entity body = scene.body(makeBody(1.f, V(4.f, 0.f), V(2.f, 5.f)), &ground);

        expect(scene.forceAfterStep(body) == V(2.f, 5.f));
    };

    "a kinematic body gets no friction"_test = [=] {
        Scene scene;
        const ECS::Entity platform = scene.body(makeBody(0.f, V(5.f, 0.f)), &ground);

        expect(scene.forceAfterStep(platform) == V::zero());
    };

    "friction acts on the velocity relative to a moving surface"_test = [] {
        Scene scene;
        const SurfaceContact belt = makeContact(Up, 0.5f, 0.3f, V(2.f, 0.f));
        const ECS::Entity body = scene.body(makeBody(1.f), &belt);

        // At rest on a belt moving right: it slides left relative to the belt, so it is dragged right.
        expect(near(scene.forceAfterStep(body), V(3.f, -10.f), 1e-4f));
    };
}

void test_slope() {
    // A 30 degree slope rising to the right. Weight 10 N: 8.66 N presses on the slope, 5 N pulls down along it.
    const V normal{-0.5f, std::sqrt(3.f) / 2.f};
    const V downSlope{-std::sqrt(3.f) / 2.f, -0.5f};

    "on a gentle enough slope, static friction cancels the pull along it"_test = [=] {
        Scene scene;
        const SurfaceContact slope = makeContact(normal, 0.7f, 0.5f);  // holds up to 6.06 N
        const ECS::Entity body = scene.body(makeBody(1.f), &slope);

        const V force = scene.forceAfterStep(body);

        expect(near(Physic::tangentialComponent(force, normal), V::zero(), 1e-5f));
        expect(near(Physic::normalForce(force, normal), 8.660f, 1e-3f));
    };

    "on a slope too steep for static friction, the body is pulled down by m g (sin - muKinetic cos)"_test = [=] {
        Scene scene;
        const SurfaceContact slope = makeContact(normal, 0.3f, 0.2f);  // holds up to 2.6 N only
        const ECS::Entity body = scene.body(makeBody(1.f), &slope);

        const V along = Physic::tangentialComponent(scene.forceAfterStep(body), normal);

        // 10 * (0.5 - 0.2 * 0.866) = 3.268 N, down the slope.
        expect(near(along, downSlope * 3.268f, 1e-3f));
    };
}
}

void run_friction_system_tests() {
    test_flat_ground();
    test_slope();
}
