#pragma once

/**
 * @file
 * @brief Adds the gravity forces to every RigidBody.
 */

#include <string>

#include "ECS/ECS.hpp"
#include "Engine/Common/Physic/Forces/Gravity.hpp"
#include "Engine/Component/Physics/GravityAttractor.hpp"
#include "Engine/Component/Physics/RigidBody.hpp"
#include "Engine/Component/Transform.hpp"
#include "GravitySettings.hpp"

namespace Engine::System {
    /**
     * @brief Adds gravity to the forces of every RigidBody.
     *
     * Two sources, both read from the GravitySettings resource:
     * - uniform gravity: the weight m * g * gravityScale of every RigidBody;
     * - universal gravitation: each GravityAttractor pulls every RigidBody
     *   that has a Transform, with F = G * mA * mB / d².
     *
     * It only adds forces: register IntegrationSystem after it (and after
     * every other force system) to turn them into motion.
     */
    class GravitySystem : public ECS::ISystem {
        public:
            /** @brief Sets a default GravitySettings resource when none was set. */
            void init(ECS::Registry& world) override {
                if (!world.hasResource<GravitySettings>()) {
                    world.setResource<GravitySettings>();
                }
            }

            void update(ECS::Registry& world, double) override {
                const GravitySettings& settings = world.getResource<GravitySettings>();
                applyUniformGravity(world, settings);
                applyAttractors(world, settings);
            }

            const std::string name() const override { return "GravitySystem"; }

        private:
            static void applyUniformGravity(ECS::Registry& world, const GravitySettings& settings) {
                for (auto [entity, body] : world.view<Component::RigidBody>()) {
                    body.force += Physic::weight(body.mass, settings.gravity, body.gravityScale);
                }
            }

            static void applyAttractors(ECS::Registry& world, const GravitySettings& settings) {
                for (auto [attractor, attractorTransform, field] :
                     world.view<Component::Transform, Component::GravityAttractor>()) {
                    for (auto [entity, transform, body] :
                         world.view<Component::Transform, Component::RigidBody>()) {
                        // An attractor that is also a body is not pulled by itself.
                        if (entity == attractor) {
                            continue;
                        }
                        body.force += Physic::gravitationalForce(
                            transform.position, body.mass,
                            attractorTransform.position, field.mass,
                            settings.gravitationalConstant);
                    }
                }
            }
    };
}
