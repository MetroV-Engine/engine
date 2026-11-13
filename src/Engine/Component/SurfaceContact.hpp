#pragma once

/**
 * @file
 * @brief The surface a body is resting on this step, and how much it grips.
 */

#include "Engine/Common/Math/Vector.hpp"

namespace Engine::Component {
    /**
     * @brief The surface a RigidBody touches this step: FrictionSystem slows
     *        the body along it.
     *
     * Present only while the body touches the surface. CollisionResponseSystem
     * adds it to every dynamic body that touches a Collider, updates it each
     * step and removes it when the body leaves; without that system, game
     * code can add it itself. One surface per body: the one it stands on.
     *
     * Friction only: the surface's support, which keeps the body from going
     * through it, comes from the collision response.
     */
    struct SurfaceContact {
        /**
         * @brief Unit normal of the surface (length 1), pointing out of it,
         *        towards the body: (0, 1) for flat ground with Y up.
         */
        Math::Vector2f normal{0.f, 1.f};

        /**
         * @brief Static coefficient muStatic, >= 0: a resting body stays still
         *        while the force along the surface is at most muStatic * N.
         *
         * About 0.1 on ice, 0.5 for wood on wood, 1 for rubber on concrete.
         */
        float staticFriction = 0.5f;

        /**
         * @brief Kinetic coefficient muKinetic, >= 0, usually below
         *        muStatic: a sliding body is slowed by muKinetic * N.
         *
         * About 0.03 on ice, 0.3 for wood on wood, 0.8 for rubber on concrete.
         */
        float kineticFriction = 0.3f;

        /**
         * @brief Velocity of the surface itself, in m/s: a moving platform or
         *        a conveyor belt. Friction acts on the body's velocity
         *        relative to it, so it drags the body along.
         */
        Math::Vector2f surfaceVelocity;
    };
}
