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

#include <algorithm>

#include "Shapes.hpp"

namespace Engine::Collision {
    namespace Detail {
        /** @brief The span [min, max] a shape covers along an axis (not necessarily of length 1). */
        struct Interval {
            float min = 0.f;
            float max = 0.f;
        };

        /**
         * @brief The unit outward normal of edge i in normal, or false for an
         *        edge of length 0. A valid Polygon has none, but a Rectangle
         *        of size 0 (a segment, a point) given as a polygon does: its
         *        other edges still give the axes that matter.
         */
        [[nodiscard]] inline bool edgeNormal(const Polygon& polygon, std::size_t i, Vector2f& normal) noexcept {
            const Vector2f along = polygon.edge(i);
            if (along.x == 0.f && along.y == 0.f) {
                return false;
            }
            normal = polygon.normal(i);
            return true;
        }

        /** @brief The span of the polygon's vertices along axis. */
        [[nodiscard]] constexpr Interval project(const Polygon& polygon, const Vector2f& axis) noexcept {
            Interval span{polygon.vertices[0].dot(axis), polygon.vertices[0].dot(axis)};
            for (std::size_t i = 1; i < polygon.count; ++i) {
                const float along = polygon.vertices[i].dot(axis);
                span.min = std::min(span.min, along);
                span.max = std::max(span.max, along);
            }
            return span;
        }

        /**
         * @brief Whether one of the edges of `edges` separates a from b: along
         *        its normal, their spans do not meet (separating axis theorem).
         *
         * The normals are not normalized: dividing every projection by the
         * same length changes no comparison, and saves a square root per edge.
         */
        [[nodiscard]] constexpr bool separatedByAnEdgeOf(const Polygon& edges, const Polygon& a, const Polygon& b) noexcept {
            for (std::size_t i = 0; i < edges.count; ++i) {
                const Vector2f along = edges.edge(i);
                const Vector2f axis{along.y, -along.x};
                const Interval spanA = project(a, axis);
                const Interval spanB = project(b, axis);
                if (spanA.max < spanB.min || spanB.max < spanA.min) {
                    return true;
                }
            }
            return false;
        }
    }

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

    /** @brief Whether point is in the polygon, edges included. */
    [[nodiscard]] constexpr bool contains(const Polygon& polygon, const Vector2f& point) noexcept {
        return polygon.contains(point);
    }

    /**
     * @brief Whether two convex polygons overlap or touch.
     *
     * Separating axis theorem: two convex shapes are apart exactly when
     * some line separates them, and for polygons one of their edges gives
     * it. So they overlap when no edge normal of either one separates their
     * spans.
     */
    [[nodiscard]] constexpr bool overlaps(const Polygon& a, const Polygon& b) noexcept {
        return !Detail::separatedByAnEdgeOf(a, a, b) && !Detail::separatedByAnEdgeOf(b, a, b);
    }

    /** @brief Whether a polygon and a rectangle overlap or touch: the rectangle as a polygon. */
    [[nodiscard]] constexpr bool overlaps(const Polygon& polygon, const Rectangle& rectangle) noexcept {
        return overlaps(polygon, Polygon::fromRectangle(rectangle));
    }

    /** @copydoc overlaps(const Polygon&, const Rectangle&) */
    [[nodiscard]] constexpr bool overlaps(const Rectangle& rectangle, const Polygon& polygon) noexcept {
        return overlaps(polygon, rectangle);
    }

    /**
     * @brief Whether a polygon and a circle overlap or touch: the circle
     *        reaches the polygon's closest point.
     *
     * Computed around the circle's center, as collide() does, so that the
     * two always agree: far from the origin, the closest point on a
     * slanted edge is only known to a float step of the coordinates.
     */
    [[nodiscard]] constexpr bool overlaps(const Polygon& polygon, const Circle& circle) noexcept {
        return polygon.translated(-circle.center).closestPoint(Vector2f::zero()).lengthSquared()
            <= circle.radius * circle.radius;
    }

    /** @copydoc overlaps(const Polygon&, const Circle&) */
    [[nodiscard]] constexpr bool overlaps(const Circle& circle, const Polygon& polygon) noexcept {
        return overlaps(polygon, circle);
    }
}
