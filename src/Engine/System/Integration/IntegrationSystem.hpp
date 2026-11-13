#pragma once

/**
 * @file
 * @brief Turns the forces of the step into motion.
 */

#include <string>

#include "ECS/ECS.hpp"
#include "Engine/Common/Physic/Forces/Dynamics.hpp"
#include "Engine/Common/Physic/Kinematics.hpp"
#include "Engine/Common/Physic/Rotation.hpp"
#include "Engine/Component/Collider.hpp"
#include "Engine/Component/RigidBody.hpp"
#include "Engine/Component/Transform.hpp"

namespace Engine::System {
    /**
     * @brief Moves every RigidBody that has a Transform, then clears the
     *        forces of the step.
     *
     * a = force / mass, then semi-implicit Euler: velocity first, then
     * position (see Physic::integrate). The same for rotation: angular
     * acceleration = torque / I (I from the Collider), then the angular
     * velocity, then the rotation (Physic::integrateRotation). A kinematic
     * body (mass 0) keeps turning at its angular velocity; a fixedRotation
     * one keeps its rotation. Register it after every force
     * system (GravitySystem, SpringSystem, ..., then FrictionSystem), so
     * that it sees all of the step's forces, and call SystemManager::update
     * with a fixed dt: with a variable one, jump heights change with the
     * frame rate.
     */
    class IntegrationSystem : public ECS::ISystem {
        public:
            void update(ECS::Registry& world, double dt) override {
                const float step = static_cast<float>(dt);
                for (auto [entity, transform, body] :
                     world.view<Component::Transform, Component::RigidBody>()) {
                    const Math::Vector2f acceleration = Physic::accelerationFromForce(body.force, body.mass);
                    Physic::integrate(transform.position, body.velocity, acceleration, step);
                    if (body.fixedRotation) {
                        continue;
                    }
                    const Component::Collider* collider = world.hasComponents<Component::Collider>(entity)
                        ? &world.getComponent<Component::Collider>(entity)
                        : nullptr;
                    const float angularAcceleration = body.torque * Component::inverseInertia(body, collider);
                    Physic::integrateRotation(transform.rotation, body.angularVelocity, angularAcceleration, step);
                }
                // Every body, with a Transform or not, starts the next step with no force.
                for (auto [entity, body] : world.view<Component::RigidBody>()) {
                    body.force = {};
                    body.torque = 0.f;
                }
            }

            const std::string name() const override { return "IntegrationSystem"; }
    };
}
