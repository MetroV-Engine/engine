#pragma once

/**
 * @file
 * @brief Ray casts: where a straight motion first meets a shape.
 *
 * A ray goes from origin to origin + delta: delta is the direction times
 * the maximum distance. Only a ray that enters the shape from outside
 * during that motion hits it. A ray starting inside a shape ignores it, as
 * in Box2D, Unity or Godot: a body can cast rays from its own center
 * through its own collider. A ray starting exactly on the surface and
 * moving inwards hits at fraction 0.
 */

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <utility>

#include "Shapes/Circle.hpp"
#include "Hit.hpp"
#include "Shapes/Rectangle.hpp"

namespace Engine::Collision {
    /**
     * @brief First hit of the ray from origin to origin + delta on a rectangle.
     *
     * Slab method: the ray is in the rectangle while it is both between
     * the two vertical sides and between the two horizontal ones. On each
     * axis it enters that band at one time and leaves it at another; it is
     * in the rectangle from the latest entry to the earliest exit. A corner
     * hit exactly (both entries equal) reports the horizontal side, like
     * collide().
     */
    [[nodiscard]] constexpr std::optional<Hit> raycast(
        const Vector2f& origin, const Vector2f& delta, const Rectangle& rectangle) noexcept {
        float enter = -std::numeric_limits<float>::infinity();
        float exit = std::numeric_limits<float>::infinity();
        Vector2f normal;

        if (delta.x == 0.f) {
            // Parallel to the vertical sides: in the band for the whole motion, or never.
            if (origin.x < rectangle.min.x || origin.x > rectangle.max.x) {
                return std::nullopt;
            }
        } else {
            const float inverse = 1.f / delta.x;
            float bandEnter = (rectangle.min.x - origin.x) * inverse;
            float bandExit = (rectangle.max.x - origin.x) * inverse;
            float side = -1.f;  // moving right: enters through the left side
            if (bandEnter > bandExit) {
                std::swap(bandEnter, bandExit);
                side = 1.f;
            }
            enter = bandEnter;
            exit = bandExit;
            normal = {side, 0.f};
        }

        if (delta.y == 0.f) {
            if (origin.y < rectangle.min.y || origin.y > rectangle.max.y) {
                return std::nullopt;
            }
        } else {
            const float inverse = 1.f / delta.y;
            float bandEnter = (rectangle.min.y - origin.y) * inverse;
            float bandExit = (rectangle.max.y - origin.y) * inverse;
            float side = -1.f;  // moving up: enters through the bottom side
            if (bandEnter > bandExit) {
                std::swap(bandEnter, bandExit);
                side = 1.f;
            }
            if (bandEnter >= enter) {
                enter = bandEnter;
                normal = {0.f, side};
            }
            exit = std::min(exit, bandExit);
        }

        // Misses the rectangle, starts inside it (enter < 0), or does not reach it.
        if (enter > exit || enter < 0.f || enter > 1.f) {
            return std::nullopt;
        }
        return Hit{enter, origin + delta * enter, normal};
    }

    /**
     * @brief First hit of the ray from origin to origin + delta on a circle.
     *
     * Solves |origin + t * delta - center|² = radius² for its first root:
     * a t² + 2 b t + c = 0, with m = origin - center, a = delta . delta,
     * b = m . delta and c = m . m - radius².
     *
     * Two textbook steps are rewritten:
     * - The discriminant b² - a c is computed as a r² - (m x delta)², the
     *   same value (Lagrange's identity: |m|² |delta|² - (m . delta)² =
     *   (m x delta)²). b² and a c are huge and nearly equal when the circle
     *   is small and far: subtracting them lost up to 1e-4 at 7 m, this
     *   form loses about 1e-7.
     * - The first root (-b - √Δ) / a is computed as c / (-b + √Δ), the same
     *   value (the two roots multiply to c / a). With c >= 0 and -b > 0 it
     *   is >= 0 by construction, where the first form would need a clamp:
     *   Δ is no longer computed from b, so it may round a hair above b².
     */
    [[nodiscard]] inline std::optional<Hit> raycast(
        const Vector2f& origin, const Vector2f& delta, const Circle& circle) noexcept {
        const Vector2f fromCenter = origin - circle.center;
        const float radiusSquared = circle.radius * circle.radius;
        const float c = fromCenter.lengthSquared() - radiusSquared;
        const float b = fromCenter.dot(delta);
        // Starts inside, or does not move towards the circle.
        if (c < 0.f || b >= 0.f) {
            return std::nullopt;
        }
        const float cross = fromCenter.cross(delta);
        const float discriminant = delta.lengthSquared() * radiusSquared - cross * cross;
        if (discriminant < 0.f) {
            return std::nullopt;  // passes beside the circle
        }
        // c >= 0 and -b > 0: the fraction is >= 0, and 0 when starting on the surface.
        const float fraction = c / (-b + std::sqrt(discriminant));
        if (fraction > 1.f) {
            return std::nullopt;
        }
        const Vector2f point = origin + delta * fraction;
        // Not normalized(), which gives zero below its epsilon: a tiny circle still has a normal.
        // Radius 0 (a point) has no surface: the normal then faces the ray.
        const Vector2f outward = point - circle.center;
        const float length = outward.length();
        // Divided per coordinate, not with Vector2's / (a multiply by 1 / length, off by
        // a float step): on a side, (0, d) / d must be exactly (0, 1).
        if (length > 0.f) {
            return Hit{fraction, point, {outward.x / length, outward.y / length}};
        }
        const float speed = delta.length();
        return Hit{fraction, point, {-delta.x / speed, -delta.y / speed}};
    }
}
