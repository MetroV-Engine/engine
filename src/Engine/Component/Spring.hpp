#pragma once

/**
 * @file
 * @brief A damped spring between two ends: a body and another entity, or a
 *        body and a fixed point.
 */

#include "ECS/Core/Entity/Entity.hpp"
#include "Engine/Common/Math/Vector.hpp"

namespace Engine::Component {
    /**
     * @brief A damped spring (Hooke's law) linking `body` to `other`, or to
     *        the fixed point `anchor` when `other` is left as no entity.
     *
     * SpringSystem pulls both ends back towards `restLength`: grappling
     * hooks and ropes, bouncy platforms, hanging lanterns, chains of bodies.
     * The spring names its ends, so it can live on an entity of its own: a
     * body can then have several springs (a plank held at both ends, a link
     * in the middle of a chain).
     *
     * Each end needs a Transform: the spring is attached at its position.
     * An end with a RigidBody is moved by the spring; one without (a hook in
     * a wall) only holds it. When an end is killed, the spring is broken
     * and does nothing.
     *
     * Stiff springs need small steps. With semi-implicit Euler, a body of
     * mass m on a spring to a fixed point is stable while
     * dt * sqrt(k / m) < 2 * (sqrt(1 + z^2) - z), with z = c / (2 sqrt(k m))
     * the damping ratio: 2 undamped, 0.83 critically damped. At 60 Hz, k
     * below about 14,000 m undamped, but 2,400 m critically damped. Between
     * two bodies, m is their reduced mass m1 m2 / (m1 + m2); a body between
     * two springs of a chain feels up to 4 k and 4 c.
     */
    struct Spring {
        /** @brief The first end: the body the spring pulls. */
        ECS::Entity body;

        /**
         * @brief The second end, or no entity (the default): then the second
         *        end is the fixed point `anchor`.
         */
        ECS::Entity other;

        /** @brief The second end when `other` is no entity, in world space, in m. */
        Math::Vector2f anchor;

        /** @brief Length at which the spring exerts no force, in m, >= 0. */
        float restLength = 1.f;

        /** @brief Stiffness k, in N/m, >= 0: how hard it pulls per metre of stretch. */
        float stiffness = 100.f;

        /**
         * @brief Damping c, in N·s/m, >= 0: how fast the oscillations die out.
         *
         * 0 oscillates forever. Physic::criticalDamping(k, m) settles as fast
         * as possible without overshooting; between two bodies, use for m
         * their reduced mass m1 * m2 / (m1 + m2).
         */
        float damping = 0.f;
    };
}
