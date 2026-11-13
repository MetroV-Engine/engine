#pragma once

/**
 * @file
 * @brief Settings of the collision response, as an ECS resource.
 */

namespace Engine::System {
    /**
     * @brief Resource read by CollisionResponseSystem. It sets a default one
     *        in init() when none was set.
     */
    struct CollisionSettings {
        /**
         * @brief Closing speed under which an impact does not bounce, in
         *        m/s, >= 0 (Physic::bounceImpulse's restingSpeed).
         *
         * A body resting on the ground falls into it by g * dt every step
         * (0.16 m/s at 60 Hz under 9.81 m/s²); bouncing that back would make
         * it hop on the spot forever. 0.5 m/s is three times that, and only
         * cancels bounces lower than 1.3 cm. Raise it with a stronger
         * gravity or a longer step.
         */
        float restingSpeed = 0.5f;
    };
}
