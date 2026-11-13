#pragma once

/**
 * @file
 * @brief Makes Buoyant bodies float in FluidVolumes.
 */

#include <cmath>
#include <string>
#include <variant>

#include "ECS/ECS.hpp"
#include "Engine/Common/Physic/Forces/Buoyancy.hpp"
#include "Engine/Common/Physic/Forces/Drag.hpp"
#include "Engine/Common/Physic/Rotation.hpp"
#include "Engine/Common/Shapes/Rectangle.hpp"
#include "Engine/Component/Physics/Buoyant.hpp"
#include "Engine/Component/Physics/Collider.hpp"
#include "Engine/Component/Physics/FluidVolume.hpp"
#include "Engine/Component/Physics/RigidBody.hpp"
#include "Engine/Component/Transform.hpp"
#include "Engine/System/Physics/Gravity/GravitySettings.hpp"

namespace Engine::System {
    /**
     * @brief For each FluidVolume and each Buoyant RigidBody in it:
     * - buoyancy: the part of the Buoyant shape inside the fluid's box and
     *   under its surface (Physic::submergedPart), its area A pushed up by
     *   Physic::buoyancyForce at its centroid, the center of buoyancy. Off
     *   the center of mass, the push also turns the body: a raft tilted on
     *   one side has more of that side under water, which pushes it level;
     * - the fluid's drag on that area: linear on the velocity
     *   (FluidVolume::drag * A), and the same decay on the spin, so that a
     *   body stops rocking.
     *
     * The buoyancy opposes the gravity of the GravitySettings resource (the
     * default one when none is set), so it should run with GravitySystem.
     * It only adds forces and torques: register it with the other force
     * systems, before FrictionSystem and IntegrationSystem
     * (Physics::install does).
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
                    const Shapes::Rectangle box = Shapes::Rectangle::fromCenter(volumeTransform.position, fluid.halfSize);
                    for (auto [entity, transform, body, buoyant] :
                         world.view<Component::Transform, Component::RigidBody, Component::Buoyant>()) {
                        const Physic::SubmergedPart part = submergedPart(buoyant.worldShape(transform), box);
                        if (part.area <= 0.f) {
                            continue;
                        }
                        const Math::Vector2f push = Physic::buoyancyForce(fluid.density, part.area, gravity);
                        body.force += push;
                        body.torque += Physic::torque(part.centroid - transform.position, push);

                        const float resistance = fluid.drag * part.area;
                        body.force += Physic::linearDragForStep(body.velocity, resistance, body.mass, step);
                        body.torque += angularDragForStep(world, entity, body, resistance, step);
                    }
                }
            }

            const std::string name() const override { return "BuoyancySystem"; }

            /** @brief The part of a placed shape inside the fluid's box: Physic::submergedPart of its form. */
            [[nodiscard]] static Physic::SubmergedPart submergedPart(
                const Component::Collider::WorldShape& shape, const Shapes::Rectangle& fluid) noexcept {
                return std::visit([&fluid](const auto& placed) { return Physic::submergedPart(placed, fluid); }, shape);
            }

        private:
            /**
             * @brief Torque that slows the spin as linear drag slows the
             *        motion: the angular velocity decays by exp(-c dt / m)
             *        over the step, exactly, as linearDragForStep does for
             *        the velocity. None for a body that does not turn.
             */
            static float angularDragForStep(
                const ECS::Registry& world, ECS::Entity entity, const Component::RigidBody& body, float resistance, float step) {
                const Component::Collider* collider = world.hasComponents<Component::Collider>(entity)
                    ? &world.getComponent<Component::Collider>(entity)
                    : nullptr;
                const float inverseInertia = Component::inverseInertia(body, collider);
                if (inverseInertia <= 0.f || step <= 0.f) {
                    return 0.f;
                }
                const float kept = std::exp(-resistance * step / body.mass);
                return body.angularVelocity * (kept - 1.f) / (step * inverseInertia);
            }
    };
}
