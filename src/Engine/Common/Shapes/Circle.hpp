#pragma once

/**
 * @file
 * @brief Circle: the round shape.
 */

#include "../Math/Vector.hpp"
#include "Rectangle.hpp"

namespace Engine::Shapes {
    /** @brief Disc of the given center and radius, in world space and meters. */
    struct Circle {
        Vector2f center;
        float radius = 0.f;

        /** @brief The smallest rectangle containing the circle. */
        [[nodiscard]] constexpr Rectangle boundingBox() const noexcept {
            return Rectangle::fromCenter(center, {radius, radius});
        }

        [[nodiscard]] constexpr bool operator==(const Circle& other) const noexcept = default;
    };
}
