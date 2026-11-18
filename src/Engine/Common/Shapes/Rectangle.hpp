#pragma once

/**
 * @file
 * @brief Rectangle: the axis-aligned box.
 */

#include <algorithm>

#include "../Math/Vector.hpp"

namespace Engine::Shapes {
    using Math::Vector2f;

    /**
     * @brief Axis-aligned rectangle [min, max], in world space and meters.
     *
     * Its edges stay parallel to the axes (an "AABB", axis-aligned
     * bounding box): it never rotates. It is stored as its two corners
     * because the overlap test and the broad phase only compare edges:
     * four comparisons, no arithmetic. Build one from a center with
     * fromCenter(). Invariant: min <= max on both axes.
     */
    struct Rectangle {
        Vector2f min;
        Vector2f max;

        /** @brief The rectangle of the given center and half size (half the width, half the height). */
        [[nodiscard]] static constexpr Rectangle fromCenter(const Vector2f& center, const Vector2f& halfSize) noexcept {
            return {center - halfSize, center + halfSize};
        }

        [[nodiscard]] constexpr Vector2f center() const noexcept { return (min + max) * 0.5f; }

        /** @brief Half the width and half the height. */
        [[nodiscard]] constexpr Vector2f halfSize() const noexcept { return (max - min) * 0.5f; }

        /** @brief Width and height. */
        [[nodiscard]] constexpr Vector2f size() const noexcept { return max - min; }

        /** @brief The same rectangle moved by offset. */
        [[nodiscard]] constexpr Rectangle translated(const Vector2f& offset) const noexcept {
            return {min + offset, max + offset};
        }

        /** @brief The rectangle grown by margin on every side (margin.x left and right, margin.y below and above). */
        [[nodiscard]] constexpr Rectangle expanded(const Vector2f& margin) const noexcept {
            return {min - margin, max + margin};
        }

        /** @brief The smallest rectangle containing both rectangles. */
        [[nodiscard]] constexpr Rectangle merged(const Rectangle& other) const noexcept {
            return {Math::min(min, other.min), Math::max(max, other.max)};
        }

        /** @brief The point of the rectangle closest to point: point itself when it is inside. */
        [[nodiscard]] constexpr Vector2f closestPoint(const Vector2f& point) const noexcept {
            return {std::clamp(point.x, min.x, max.x), std::clamp(point.y, min.y, max.y)};
        }

        [[nodiscard]] constexpr bool operator==(const Rectangle& other) const noexcept = default;
    };
}
