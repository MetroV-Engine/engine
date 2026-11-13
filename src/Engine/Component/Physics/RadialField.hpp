#pragma once

/**
 * @file
 * @brief A field that pushes bodies away from a point, or pulls them in.
 */

namespace Engine::Component {
    /**
     * @brief Pushes every RigidBody within `radius` away from the entity's
     *        position, with a strength fading linearly to 0 at the radius
     *        (Physic::radialPush).
     *
     * Needs a Transform on the same entity: the field comes from its
     * position. A negative strength pulls bodies towards the center instead:
     * a vortex, a magnet. Applied every step, it is a force: a fan, a
     * repulsor. For a one-time blast, apply Physic::radialPush once as an
     * impulse instead.
     */
    struct RadialField {
        /** @brief Force at the center, in N: > 0 pushes away, < 0 pulls in. */
        float strength = 0.f;

        /** @brief Distance at which the field fades out, in m, > 0. */
        float radius = 1.f;
    };
}
