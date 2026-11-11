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

#include <optional>

#include "Raycast.hpp"
#include "Shapes/Circle.hpp"
#include "Hit.hpp"
#include "Shapes/Rectangle.hpp"

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
}
