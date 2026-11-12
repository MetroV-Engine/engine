#pragma once

/**
 * @file
 * @brief Adds the push of every RadialField to the bodies around it.
 */

#include <string>

#include "ECS/ECS.hpp"
#include "Engine/Common/Physic/Forces/Radial.hpp"
#include "Engine/Component/RadialField.hpp"
#include "Engine/Component/RigidBody.hpp"
#include "Engine/Component/Transform.hpp"

namespace Engine::System {
    /**
     * @brief Each RadialField pushes every RigidBody with a Transform within
     *        its radius (Physic::radialPush): full strength at the center,
     *        0 at the radius.
     *
     * A field does not push its own entity. It only adds forces: register
     * it with the other force systems, before FrictionSystem and
     * IntegrationSystem.
     */
    class RadialFieldSystem : public ECS::ISystem {
        public:
            void update(ECS::Registry& world, double) override {
                for (auto [source, sourceTransform, field] :
                     world.view<Component::Transform, Component::RadialField>()) {
                    for (auto [entity, transform, body] :
                         world.view<Component::Transform, Component::RigidBody>()) {
                        if (entity == source) {
                            continue;
                        }
                        body.force += Physic::radialPush(
                            transform.position, sourceTransform.position, field.strength, field.radius);
                    }
                }
            }

            const std::string name() const override { return "RadialFieldSystem"; }
    };
}
