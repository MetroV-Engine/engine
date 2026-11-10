#pragma once

/**
 * @file
 * @brief Adds the friction of the surface each RigidBody rests on.
 */

#include <string>

#include "ECS/ECS.hpp"
#include "Engine/Common/Physic/Forces/Friction.hpp"
#include "Engine/Common/Physic/Forces/Surface.hpp"
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
     * Order matters: register it after every other force system
     * (GravitySystem, ...), since N and static friction depend on all of
     * the step's forces, and just before IntegrationSystem, since the force
     * is computed for the same dt.
     */
    class FrictionSystem : public ECS::ISystem {
        public:
            void update(ECS::Registry& world, double dt) override {
                const float step = static_cast<float>(dt);
                for (auto [entity, body, contact] : world.view<Component::RigidBody, Component::SurfaceContact>()) {
                    const float pressing = Physic::normalForce(body.force, contact.normal);
                    if (pressing <= 0.f) {
                        continue;  // nothing presses the body onto the surface
                    }
                    const Math::Vector2f velocity =
                        Physic::tangentialComponent(body.velocity - contact.surfaceVelocity, contact.normal);
                    const Math::Vector2f applied = Physic::tangentialComponent(body.force, contact.normal);
                    body.force += Physic::frictionForceForStep(
                        velocity, applied, contact.staticFriction, contact.kineticFriction, pressing, body.mass, step);
                }
            }

            const std::string name() const override { return "FrictionSystem"; }
    };
}
