#pragma once

/**
 * @file
 * @brief Adds air or water resistance to every body that has a Drag.
 */

#include <string>

#include "ECS/ECS.hpp"
#include "Engine/Common/Physic/Forces/Drag.hpp"
#include "Engine/Component/Physics/Drag.hpp"
#include "Engine/Component/Physics/RigidBody.hpp"

namespace Engine::System {
    /**
     * @brief Adds the linear and quadratic drag of every Drag to its
     *        RigidBody, against its velocity relative to the Drag's flow.
     *
     * Computed for the step (Physic::linearDragForStep,
     * quadraticDragForStep): a strong drag stops a body, it never sends it
     * backwards. Each formula alone can stop the body, so the quadratic
     * drag acts on the velocity the linear one leaves: together they still
     * stop it at most. It only adds forces: register it with the other force
     * systems, before FrictionSystem and IntegrationSystem.
     */
    class DragSystem : public ECS::ISystem {
        public:
            void update(ECS::Registry& world, double dt) override {
                const float step = static_cast<float>(dt);
                for (auto [entity, body, drag] : world.view<Component::RigidBody, Component::Drag>()) {
                    const Math::Vector2f relativeVelocity = body.velocity - drag.flow;
                    const Math::Vector2f linear = Physic::linearDragForStep(relativeVelocity, drag.linear, body.mass, step);
                    // What the linear drag leaves of the velocity at the end of the step.
                    const Math::Vector2f remaining = body.mass > Math::Epsilon<float>
                        ? relativeVelocity + linear * (step / body.mass)
                        : relativeVelocity;
                    body.force += linear + Physic::quadraticDragForStep(remaining, drag.quadratic, body.mass, step);
                }
            }

            const std::string name() const override { return "DragSystem"; }
    };
}
