#pragma once

/**
 * @file
 * @brief Continuous collision: where a moving shape first touches a still one.
 *
 * Tested only once per frame, a fast body can cross a thin wall without
 * ever being seen overlapping it ("tunneling"). A sweep tests the whole
 * motion instead. Each sweep reduces to a ray cast: shrinking the moving
 * shape to its center grows the still one by the same amount (Minkowski
 * sum), and the center's path is a ray.
 *
 * Same conventions as raycast(): delta is the whole motion, and the moving
 * shape moved by delta * fraction exactly touches the other one. Hit::point
 * is the moving shape's center at that moment; Hit::normal is the still
 * shape's surface normal there, pointing back at the moving one. Shapes
 * that already overlap at the start give no hit: test them with collide()
 * first.
 */

#include <algorithm>
#include <cstddef>
#include <limits>
#include <optional>
#include <utility>

#include "Overlap.hpp"
#include "Raycast.hpp"
#include "Hit.hpp"
#include "Shapes.hpp"

namespace Engine::Collision {
    /** @brief First contact of a moving rectangle with a still rectangle. */
    [[nodiscard]] constexpr std::optional<Hit> sweep(
        const Rectangle& moving, const Vector2f& delta, const Rectangle& target) noexcept {
        return raycast(moving.center(), delta, target.expanded(moving.halfSize()));
    }

    /** @brief First contact of a moving circle with a still circle. */
    [[nodiscard]] inline std::optional<Hit> sweep(
        const Circle& moving, const Vector2f& delta, const Circle& target) noexcept {
        return raycast(moving.center, delta, Circle{target.center, target.radius + moving.radius});
    }

    /**
     * @brief First contact of a moving circle with a still rectangle.
     *
     * The center must stay out of the rectangle grown by the radius, whose
     * corners are rounded: two crossed rectangles plus four corner circles.
     * The first of these six pieces the center's ray meets is the hit. A
     * plain grown rectangle would wrongly hit a circle passing near a
     * corner.
     */
    [[nodiscard]] inline std::optional<Hit> sweep(
        const Circle& moving, const Vector2f& delta, const Rectangle& target) noexcept {
        const float radius = moving.radius;
        const Vector2f& origin = moving.center;
        // Radius 0, a point: a ray. (The corner pieces would wrongly catch a point
        // that starts inside and leaves exactly through a corner.)
        if (radius <= 0.f) {
            return raycast(origin, delta, target);
        }
        // Work around the circle's start, at (0, 0). Subtracting nearby coordinates is exact
        // in float, and the pieces below (sides moved by the radius) are then rounded where
        // float steps are tiny: built in world space, 1 km out, they were off by 3e-5 m.
        const Rectangle box = target.translated(-origin);
        // Already overlapping (strictly: touching and moving inwards hits at 0).
        if (box.closestPoint(Vector2f::zero()).lengthSquared() < radius * radius) {
            return std::nullopt;
        }

        // Only the first fraction is kept, and the hit is built once at the end: copying a
        // whole Hit for each piece cost 10 to 25 % (benchmarks/BenchCollisionRecentering.cpp).
        float fraction = 2.f;  // above 1: no hit yet
        const auto keepFirst = [&fraction](const std::optional<Hit>& hit) {
            if (hit && hit->fraction < fraction) {
                fraction = hit->fraction;
            }
        };
        const Vector2f start = Vector2f::zero();
        keepFirst(raycast(start, delta, Rectangle{{box.min.x - radius, box.min.y}, {box.max.x + radius, box.max.y}}));
        keepFirst(raycast(start, delta, Rectangle{{box.min.x, box.min.y - radius}, {box.max.x, box.max.y + radius}}));
        keepFirst(raycast(start, delta, Circle{box.min, radius}));
        keepFirst(raycast(start, delta, Circle{{box.max.x, box.min.y}, radius}));
        keepFirst(raycast(start, delta, Circle{{box.min.x, box.max.y}, radius}));
        keepFirst(raycast(start, delta, Circle{box.max, radius}));
        if (fraction > 1.f) {
            return std::nullopt;
        }

        // The normal of the rounded rectangle at the contact: from the rectangle's closest
        // point to the circle's center, as in collide(). It does not depend on which piece
        // found the contact, so it is right where two pieces meet.
        const Vector2f center = delta * fraction;  // where the circle's center touches, around (0, 0)
        const Vector2f outward = center - box.closestPoint(center);
        const float length = outward.length();
        // Divided per coordinate, not with Vector2's / (a multiply by 1 / length, off by a
        // float step): on a side, (0, d) / d must be exactly (0, 1). (Length 0 only when
        // the radius underflows: then face the motion.)
        const Vector2f normal = length > 0.f ? Vector2f(outward.x / length, outward.y / length)
                                             : Vector2f(-delta.x / delta.length(), -delta.y / delta.length());
        return Hit{fraction, origin + center, normal};  // back to world space
    }

    /**
     * @brief First contact of a moving rectangle with a still circle.
     *
     * Seen from the rectangle, the circle moves by -delta: the same test as
     * a circle sweeping the rectangle, with the normal turned around.
     */
    [[nodiscard]] inline std::optional<Hit> sweep(
        const Rectangle& moving, const Vector2f& delta, const Circle& target) noexcept {
        const std::optional<Hit> hit = sweep(target, -delta, moving);
        if (!hit) {
            return std::nullopt;
        }
        return Hit{hit->fraction, moving.center() + delta * hit->fraction, -hit->normal};
    }

    /**
     * @brief First contact of a moving convex polygon with a still one.
     *
     * Separating axis theorem over time: along each edge normal of either
     * polygon, the moving span meets the still one during an interval of
     * the motion; the polygons touch from the latest of those entries to
     * the earliest exit. The normal is the axis of the latest entry,
     * turned against the motion (ties go to the axis closer to vertical).
     *
     * Computed around the moving polygon's center, so that projections
     * stay small, where float steps are tiny.
     */
    [[nodiscard]] inline std::optional<Hit> sweep(
        const Polygon& moving, const Vector2f& delta, const Polygon& target) noexcept {
        const Vector2f origin = moving.center();
        const Polygon start = moving.translated(-origin);
        const Polygon still = target.translated(-origin);
        float enter = -std::numeric_limits<float>::infinity();
        float exit = std::numeric_limits<float>::infinity();
        Vector2f normal;
        bool entersSomewhere = false;

        // False when this polygon's edges show that the two never meet.
        const auto throughAxesOf = [&](const Polygon& edges) {
            for (std::size_t i = 0; i < edges.count; ++i) {
                Vector2f axis;
                if (!Detail::edgeNormal(edges, i, axis)) {
                    continue;  // an edge of length 0: a rectangle of size 0 as a polygon
                }
                const Detail::Interval spanMoving = Detail::project(start, axis);
                const Detail::Interval spanStill = Detail::project(still, axis);
                const float speed = axis.dot(delta);
                if (speed == 0.f) {
                    // No motion along this axis: the spans meet for the whole motion, or never.
                    if (spanMoving.max < spanStill.min || spanStill.max < spanMoving.min) {
                        return false;
                    }
                    continue;
                }
                // The spans meet while spanStill.min - spanMoving.max <= speed * t <= spanStill.max - spanMoving.min.
                float meet = (spanStill.min - spanMoving.max) / speed;
                float part = (spanStill.max - spanMoving.min) / speed;
                if (meet > part) {
                    std::swap(meet, part);
                }
                // The face hit points back at the motion.
                const Vector2f facing = speed > 0.f ? -axis : axis;
                const bool flatter = std::fabs(facing.y) > std::fabs(normal.y);
                if (!entersSomewhere || meet > enter || (meet == enter && flatter)) {
                    enter = meet;
                    normal = facing;
                    entersSomewhere = true;
                }
                exit = std::min(exit, part);
            }
            return true;
        };
        if (!throughAxesOf(start) || !throughAxesOf(still)) {
            return std::nullopt;
        }
        // Already overlapping (enter < 0, or no motion at all), apart for the whole motion, or too far.
        if (!entersSomewhere || enter > exit || enter < 0.f || enter > 1.f) {
            return std::nullopt;
        }
        return Hit{enter, origin + delta * enter, normal};
    }

    /** @brief First contact of a moving rectangle with a still polygon: the rectangle as a polygon. */
    [[nodiscard]] inline std::optional<Hit> sweep(
        const Rectangle& moving, const Vector2f& delta, const Polygon& target) noexcept {
        return sweep(Polygon::fromRectangle(moving), delta, target);
    }

    /** @brief First contact of a moving polygon with a still rectangle: the rectangle as a polygon. */
    [[nodiscard]] inline std::optional<Hit> sweep(
        const Polygon& moving, const Vector2f& delta, const Rectangle& target) noexcept {
        return sweep(moving, delta, Polygon::fromRectangle(target));
    }

    /**
     * @brief First contact of a moving circle with a still polygon.
     *
     * As for a rectangle: the center must stay out of the polygon grown by
     * the radius, with rounded vertices. That shape is the union of one
     * band per edge (the edge pushed out by the radius) and one circle per
     * vertex; the first of these pieces the center's ray meets is the hit.
     */
    [[nodiscard]] inline std::optional<Hit> sweep(
        const Circle& moving, const Vector2f& delta, const Polygon& target) noexcept {
        const float radius = moving.radius;
        const Vector2f& origin = moving.center;
        if (radius <= 0.f) {
            return raycast(origin, delta, target);
        }
        // Around the circle's start, at (0, 0), as for the rectangle.
        const Polygon shape = target.translated(-origin);
        if (shape.closestPoint(Vector2f::zero()).lengthSquared() < radius * radius) {
            return std::nullopt;  // already overlapping (strictly)
        }

        float fraction = 2.f;  // above 1: no hit yet
        const auto keepFirst = [&fraction](const std::optional<Hit>& hit) {
            if (hit && hit->fraction < fraction) {
                fraction = hit->fraction;
            }
        };
        const Vector2f start = Vector2f::zero();
        for (std::size_t i = 0; i < shape.count; ++i) {
            const Vector2f out = shape.normal(i) * radius;
            const Vector2f& from = shape.vertex(i);
            const Vector2f& to = shape.vertex(i + 1);
            // The band of edge i, counter-clockwise: the edge, then pushed out by the radius.
            keepFirst(raycast(start, delta, Polygon{from, from + out, to + out, to}));
            keepFirst(raycast(start, delta, Circle{from, radius}));
        }
        if (fraction > 1.f) {
            return std::nullopt;
        }

        // The normal of the rounded polygon at the contact: from the polygon's closest point
        // to the circle's center, as in collide(). It does not depend on which piece found
        // the contact, so it is right where two pieces meet.
        const Vector2f center = delta * fraction;
        const Vector2f outward = center - shape.closestPoint(center);
        const float length = outward.length();
        const Vector2f normal = length > 0.f ? Vector2f(outward.x / length, outward.y / length)
                                             : Vector2f(-delta.x / delta.length(), -delta.y / delta.length());
        return Hit{fraction, origin + center, normal};
    }

    /**
     * @brief First contact of a moving polygon with a still circle: the
     *        circle sweeping the polygon by -delta, the normal turned around.
     */
    [[nodiscard]] inline std::optional<Hit> sweep(
        const Polygon& moving, const Vector2f& delta, const Circle& target) noexcept {
        const std::optional<Hit> hit = sweep(target, -delta, moving);
        if (!hit) {
            return std::nullopt;
        }
        return Hit{hit->fraction, moving.center() + delta * hit->fraction, -hit->normal};
    }
}
