#pragma once

/**
 * @file
 * @brief Hit: where a ray or a moving shape first meets a shape.
 */

#include "../../Math/Vector.hpp"

namespace Engine::Collision {
    using Math::Vector2f;

    /** @brief Where a ray (Raycast.hpp) or a moving shape (Sweep.hpp) first meets a shape. */
    struct Hit {
        /** @brief Share of the motion done at the hit, in [0, 1]: the hit is at origin + delta * fraction. */
        float fraction = 0.f;

        /** @brief For a ray, the hit point on the surface; for a sweep, the moving shape's center at impact. */
        Vector2f point;

        /** @brief Unit normal of the hit surface, pointing back towards where the motion came from. */
        Vector2f normal;
    };
}
