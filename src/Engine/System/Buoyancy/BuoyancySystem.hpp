#pragma once

/**
 * @file
 * @brief Makes Buoyant bodies float in FluidVolumes.
 */

#include <algorithm>
#include <string>

#include "ECS/ECS.hpp"
#include "Engine/Common/Physic/Forces/Buoyancy.hpp"
#include "Engine/Common/Physic/Forces/Drag.hpp"
#include "Engine/Component/Buoyant.hpp"
#include "Engine/Component/FluidVolume.hpp"
#include "Engine/Component/RigidBody.hpp"
#include "Engine/Component/Transform.hpp"
#include "Engine/System/Gravity/GravitySettings.hpp"

namespace Engine::System {
    /**
     * @brief For each FluidVolume and each Buoyant RigidBody in it: the
     *        buoyancy of the area under the surface (Physic::buoyancyForce),
     *        and the fluid's drag on that area.
     *
     * The buoyancy opposes the gravity of the GravitySettings resource (the
     * default one when none is set), so it should run with GravitySystem.
     * It only adds forces: register it with the other force systems, before
     * FrictionSystem and IntegrationSystem.
     */
    class BuoyancySystem : public ECS::ISystem {
        public:
            void update(ECS::Registry& world, double dt) override {
                const Math::Vector2f gravity = world.hasResource<GravitySettings>()
                    ? world.getResource<GravitySettings>().gravity
                    : GravitySettings{}.gravity;
                const float step = static_cast<float>(dt);
                for (auto [volume, volumeTransform, fluid] :
                     world.view<Component::Transform, Component::FluidVolume>()) {
                    for (auto [entity, transform, body, buoyant] :
                         world.view<Component::Transform, Component::RigidBody, Component::Buoyant>()) {
                        const float area = submergedArea(transform.position, buoyant.halfSize,
                                                         volumeTransform.position, fluid.halfSize);
                        if (area <= 0.f) {
                            continue;
                        }
                        body.force += Physic::buoyancyForce(fluid.density, area, gravity);
                        body.force += Physic::linearDragForStep(body.velocity, fluid.drag * area, body.mass, step);
                    }
                }
            }

            const std::string name() const override { return "BuoyancySystem"; }

            /**
             * @brief Area of the body's box inside the fluid's box and under
             *        its surface (its top side), in m².
             */
            [[nodiscard]] static float submergedArea(
                const Math::Vector2f& bodyCenter, const Math::Vector2f& bodyHalfSize,
                const Math::Vector2f& fluidCenter, const Math::Vector2f& fluidHalfSize) noexcept {
                const float width = std::min(bodyCenter.x + bodyHalfSize.x, fluidCenter.x + fluidHalfSize.x)
                                  - std::max(bodyCenter.x - bodyHalfSize.x, fluidCenter.x - fluidHalfSize.x);
                const float bottom = std::max(bodyCenter.y - bodyHalfSize.y, fluidCenter.y - fluidHalfSize.y);
                const float top = bodyCenter.y + bodyHalfSize.y;
                if (width <= 0.f || top <= bottom) {
                    return 0.f;  // beside the fluid, or below its bottom
                }
                return Physic::submergedBoxArea(bottom, top, width, fluidCenter.y + fluidHalfSize.y);
            }
    };
}
