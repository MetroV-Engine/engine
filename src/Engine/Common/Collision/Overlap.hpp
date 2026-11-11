#pragma once

/**
 * @file
 * @brief Yes/no tests: is a point in a shape, do two shapes overlap?
 *
 * Edges are included: two shapes that only touch overlap, so a body resting
 * exactly on the ground is still reported as touching it. These tests build
 * no contact and take no square root: use them when the answer is all that
 * is needed, and collide() (Collide.hpp) when the response needs a normal
 * and a depth.
 */

#include "Shapes/Circle.hpp"
#include "Shapes/Rectangle.hpp"

namespace Engine::Collision {
    /** @brief Whether point is in the rectangle, edges included. */
    [[nodiscard]] constexpr bool contains(const Rectangle& rectangle, const Vector2f& point) noexcept {
        return rectangle.min.x <= point.x && point.x <= rectangle.max.x
            && rectangle.min.y <= point.y && point.y <= rectangle.max.y;
    }

    /** @brief Whether point is in the circle, edge included. */
    [[nodiscard]] constexpr bool contains(const Circle& circle, const Vector2f& point) noexcept {
        return (point - circle.center).lengthSquared() <= circle.radius * circle.radius;
    }

    /** @brief Whether two rectangles overlap or touch. */
    [[nodiscard]] constexpr bool overlaps(const Rectangle& a, const Rectangle& b) noexcept {
        return a.min.x <= b.max.x && b.min.x <= a.max.x
            && a.min.y <= b.max.y && b.min.y <= a.max.y;
    }

    /** @brief Whether two circles overlap or touch. Compares squared distances: no square root. */
    [[nodiscard]] constexpr bool overlaps(const Circle& a, const Circle& b) noexcept {
        const float radii = a.radius + b.radius;
        return (b.center - a.center).lengthSquared() <= radii * radii;
    }

    /**
     * @brief Whether a rectangle and a circle overlap or touch.
     *
     * Exact near the corners: the circle must reach the rectangle's closest
     * point, not just its bounding box.
     */
    [[nodiscard]] constexpr bool overlaps(const Rectangle& rectangle, const Circle& circle) noexcept {
        return contains(circle, rectangle.closestPoint(circle.center));
    }

    /** @copydoc overlaps(const Rectangle&, const Circle&) */
    [[nodiscard]] constexpr bool overlaps(const Circle& circle, const Rectangle& rectangle) noexcept {
        return overlaps(rectangle, circle);
    }
}
