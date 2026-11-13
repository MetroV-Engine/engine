#pragma once

/**
 * @file
 * @brief Contact: how two overlapping shapes touch.
 */

#include "../../Math/Vector.hpp"

namespace Engine::Collision {
    using Math::Vector2f;

    /**
     * @brief How two overlapping shapes touch (Collide.hpp): what a response needs.
     *
     * Moving the second shape by normal * depth, or the first one by
     * -normal * depth, separates them exactly (they are left touching).
     */
    struct Contact {
        /** @brief Unit vector from the first shape towards the second. */
        Vector2f normal;

        /** @brief Penetration along normal, in meters: >= 0, and 0 when the shapes only touch. */
        float depth = 0.f;

        /** @brief The middle of the overlap, in world space: always inside both shapes. */
        Vector2f point;
    };
}
