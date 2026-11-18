#pragma once

/**
 * @file
 * @brief Adds the force of every Spring to the bodies at its ends.
 */

#include <string>

#include "ECS/ECS.hpp"
#include "Engine/Common/Physic/Forces/Spring.hpp"
#include "Engine/Common/Physic/Rotation.hpp"
#include "Engine/Component/Physics/RigidBody.hpp"
#include "Engine/Component/Physics/Spring.hpp"
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
     * Each end is held at Spring::bodyPoint / otherPoint, an offset that
     * turns with its Transform. Off the center, the force also turns the
     * body: its torque arm x F is added (IntegrationSystem then turns a
     * body that has a Collider and no fixedRotation), and the stretch speed
     * comes from the velocity of the held points, spin included. With both
     * points at 0, no torque: the spring of a center.
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
                    const End first = endOf(world, spring.body, spring.bodyPoint);
                    const End second = toPoint ? End{spring.anchor, {}, {}} : endOf(world, spring.other, spring.otherPoint);

                    const Math::Vector2f force = Physic::dampedSpringForce(
                        first.point, second.point, first.velocity - second.velocity,
                        spring.restLength, spring.stiffness, spring.damping);

                    pull(world, spring.body, first.arm, force);
                    if (!toPoint) {
                        pull(world, spring.other, second.arm, -force);
                    }
                }
            }

            const std::string name() const override { return "SpringSystem"; }

        private:
            /** @brief Where an end of a spring is, the arm from its body's position to it, and how fast it moves. */
            struct End {
                Math::Vector2f point;
                Math::Vector2f arm;
                Math::Vector2f velocity;
            };

            /**
             * @brief The end held at `local` (in the entity's own space): turned
             *        with the Transform, moving with the RigidBody's velocity and
             *        spin, or still for an entity without one (a hook).
             */
            static End endOf(ECS::Registry& world, ECS::Entity entity, const Math::Vector2f& local) {
                const Component::Transform& transform = world.getComponent<Component::Transform>(entity);
                const Math::Vector2f arm = transform.rotation == 0.f ? local : local.rotated(transform.rotation);
                Math::Vector2f velocity;
                if (world.hasComponents<Component::RigidBody>(entity)) {
                    const Component::RigidBody& body = world.getComponent<Component::RigidBody>(entity);
                    velocity = Physic::velocityAtPoint(body.velocity, body.angularVelocity, arm);
                }
                return {transform.position + arm, arm, velocity};
            }

            /** @brief Applies the force at the end: to the body, and its torque about the body's position. */
            static void pull(ECS::Registry& world, ECS::Entity entity, const Math::Vector2f& arm, const Math::Vector2f& force) {
                if (!world.hasComponents<Component::RigidBody>(entity)) {
                    return;
                }
                Component::RigidBody& body = world.getComponent<Component::RigidBody>(entity);
                body.force += force;
                body.torque += Physic::torque(arm, force);
            }
    };
}
