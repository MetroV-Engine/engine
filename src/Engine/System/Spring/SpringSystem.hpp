#pragma once

/**
 * @file
 * @brief Adds the force of every Spring to the bodies at its ends.
 */

#include <string>

#include "ECS/ECS.hpp"
#include "Engine/Common/Physic/Forces/Spring.hpp"
#include "Engine/Component/RigidBody.hpp"
#include "Engine/Component/Spring.hpp"
#include "Engine/Component/Transform.hpp"

namespace Engine::System {
    /**
     * @brief Adds the damped spring force of every Spring to its ends.
     *
     * For each spring: F = -k * x - c * v along the spring
     * (Physic::dampedSpringForce), with x the stretch and v the speed at
     * which it stretches, from the velocity of one end relative to the
     * other. The body gets F; the other end, when it is a RigidBody, gets
     * -F (Newton's third law), so a spring between two bodies moves their
     * center of mass by nothing.
     *
     * It only adds forces: register it with the other force systems
     * (GravitySystem, ...), before FrictionSystem, which needs all of them,
     * and before IntegrationSystem.
     */
    class SpringSystem : public ECS::ISystem {
        public:
            void update(ECS::Registry& world, double) override {
                for (auto [entity, spring] : world.view<Component::Spring>()) {
                    const bool toPoint = spring.other == ECS::Entity{};
                    if (!world.hasComponents<Component::Transform>(spring.body) ||
                        (!toPoint && !world.hasComponents<Component::Transform>(spring.other))) {
                        continue;  // an end is gone (or has no position): the spring is broken
                    }
                    const Math::Vector2f position = world.getComponent<Component::Transform>(spring.body).position;
                    const Math::Vector2f anchor =
                        toPoint ? spring.anchor : world.getComponent<Component::Transform>(spring.other).position;
                    const Math::Vector2f relativeVelocity =
                        velocityOf(world, spring.body) - (toPoint ? Math::Vector2f{} : velocityOf(world, spring.other));

                    const Math::Vector2f force = Physic::dampedSpringForce(
                        position, anchor, relativeVelocity, spring.restLength, spring.stiffness, spring.damping);

                    if (world.hasComponents<Component::RigidBody>(spring.body)) {
                        world.getComponent<Component::RigidBody>(spring.body).force += force;
                    }
                    if (!toPoint && world.hasComponents<Component::RigidBody>(spring.other)) {
                        world.getComponent<Component::RigidBody>(spring.other).force -= force;
                    }
                }
            }

            const std::string name() const override { return "SpringSystem"; }

        private:
            /** @brief Velocity of an end: its RigidBody's, or none for an end that does not move. */
            static Math::Vector2f velocityOf(ECS::Registry& world, ECS::Entity end) {
                return world.hasComponents<Component::RigidBody>(end) ? world.getComponent<Component::RigidBody>(end).velocity
                                                                      : Math::Vector2f{};
            }
    };
}
