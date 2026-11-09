#pragma once

/**
 * @file
 * @brief A body moved by physics: forces change its velocity, its velocity
 *        moves its Transform.
 */

#include "Engine/Common/Math/Vector.hpp"

namespace Engine::Component {
    /**
     * @brief Mass, velocity and the forces received during the current step.
     *
     * Needs a Transform on the same entity to move. Each step, force systems
     * (GravitySystem, ...) add to `force`; IntegrationSystem then turns it
     * into motion and clears it.
     */
    struct RigidBody {
        /**
         * @brief Mass in kg. 0 makes a kinematic body: forces do not affect
         *        it, but it still moves with its velocity (moving platforms).
         */
        float mass = 1.f;

        /**
         * @brief Multiplier of the uniform gravity for this body: 0 ignores
         *        it, above 1 falls faster.
         */
        float gravityScale = 1.f;

        /** @brief Velocity in m/s. */
        Math::Vector2f velocity;

        /** @brief Sum of the forces received this step, in N. Cleared after integration. */
        Math::Vector2f force;
    };
}
