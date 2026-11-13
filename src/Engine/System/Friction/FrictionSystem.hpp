#pragma once

/**
 * @file
 * @brief Adds the friction of the surface each RigidBody rests on.
 */

#include <string>

#include "ECS/ECS.hpp"
#include "Engine/Common/Physic/Forces/Friction.hpp"
#include "Engine/Common/Physic/Forces/Surface.hpp"
#include "Engine/Common/Physic/Impact.hpp"
#include "Engine/Common/Physic/Rotation.hpp"
#include "Engine/Component/Collider.hpp"
#include "Engine/Component/RigidBody.hpp"
#include "Engine/Component/SurfaceContact.hpp"

namespace Engine::System {
    /**
     * @brief Adds Coulomb friction to the forces of every RigidBody that has
     *        a SurfaceContact.
     *
     * For each one:
     * - N, the normal force: how hard the step's forces press the body onto
     *   the surface (its weight, on flat ground). No pressure, no friction;
     * - the velocity relative to the surface and the forces, split along it;
     * - the friction force for this step (Physic::frictionForceForStep): a
     *   resting body held by static friction stays still, a sliding one is
     *   slowed by kinetic friction and stops instead of reversing.
     *
     * With rotation, friction acts where the body touches, at
     * SurfaceContact::arm from its center: it is the contact point's
     * sliding that it stops, and the force there also turns the body. A
     * force at the point moves it by 1/m + (arm x t)² / I per newton
     * (Physic::BodyPoint::inverseMassAlong), so the step's friction is
     * computed for that effective mass: a ball on a slope is held at its
     * contact point and rolls, at 2/3 g sin(angle) for a disc. Without
     * rotation (fixedRotation, no Collider, arm 0) it is the force at the
     * center, as before.
     *
     * Order matters: register it after every other force system
     * (GravitySystem, SpringSystem, ...), since N and static friction
     * depend on all of the step's forces, and just before
     * IntegrationSystem, since the force is computed for the same dt.
     */
    class FrictionSystem : public ECS::ISystem {
        public:
            void update(ECS::Registry& world, double dt) override {
                const float step = static_cast<float>(dt);
                for (auto [entity, body, contact] : world.view<Component::RigidBody, Component::SurfaceContact>()) {
                    const float pressing = Physic::normalForce(body.force, contact.normal);
                    if (pressing <= 0.f || body.mass <= Math::Epsilon<float>) {
                        continue;  // nothing presses the body onto the surface, or nothing to slow
                    }
                    const Component::Collider* collider = world.hasComponents<Component::Collider>(entity)
                        ? &world.getComponent<Component::Collider>(entity)
                        : nullptr;
                    const Physic::BodyPoint<float> point{body.velocity, body.angularVelocity, 1.f / body.mass,
                                                         Component::inverseInertia(body, collider), contact.arm};
                    // How fast the contact point slides on the surface.
                    const Math::Vector2f sliding =
                        Physic::tangentialComponent(point.pointVelocity() - contact.surfaceVelocity, contact.normal);
                    // How the step's other forces and torques drive the point along the surface.
                    const Math::Vector2f drive = Physic::tangentialComponent(
                        body.force * point.inverseMass + contact.arm.perpendicular() * (body.torque * point.inverseInertia),
                        contact.normal);
                    // The mass the contact point has along the surface.
                    const float effectiveMass = 1.f / point.inverseMassAlong(contact.normal.perpendicular());
                    const Math::Vector2f friction = Physic::frictionForceForStep(
                        sliding, drive * effectiveMass, contact.staticFriction, contact.kineticFriction, pressing,
                        effectiveMass, step);
                    body.force += friction;
                    body.torque += Physic::torque(contact.arm, friction);
                }
            }

            const std::string name() const override { return "FrictionSystem"; }
    };
}
