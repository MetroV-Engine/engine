#pragma once

/**
 * @file
 * @brief Air or water resistance on a body.
 */

#include "Engine/Common/Math/Vector.hpp"

namespace Engine::Component {
    /**
     * @brief Slows a RigidBody moving through its surroundings: DragSystem
     *        adds a force opposing its velocity relative to `flow`.
     *
     * - `linear`: F = -c v, for slow motion in a thick fluid, or simply to
     *   make motion die out.
     * - `quadratic`: F = -c |v| v, for fast motion through air: a body of
     *   mass m falling under gravity g reaches sqrt(m g / c).
     *
     * Both can be set; 0 disables each. `flow` is the velocity of the air
     * or water itself: a wind pushes a body until it moves with it.
     */
    struct Drag {
        /** @brief Linear coefficient c, in N·s/m, >= 0. */
        float linear = 0.f;

        /** @brief Quadratic coefficient c, in N·s²/m², >= 0. */
        float quadratic = 0.f;

        /** @brief Velocity of the surrounding air or water (wind, current), in m/s. */
        Math::Vector2f flow;
    };
}
