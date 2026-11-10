#pragma once

/**
 * @file
 * @brief Narrow phase: how two overlapping shapes touch (normal, depth, point).
 *
 * collide(a, b) answers "do a and b overlap, and if so, what is the
 * shortest way to separate them?". It is the precise test run on the pairs
 * the broad phase found. For a plain yes or no, overlaps() in Overlap.hpp
 * is cheaper: no square root, no contact to build.
 */

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>

#include "Contact.hpp"
#include "Shapes/Circle.hpp"
#include "Shapes/Rectangle.hpp"

namespace Engine::Collision {
    namespace Detail {
        /**
         * @brief Middle of the overlap of [lowA, highA] and [lowB, highB], two
         * segments of the same line (given as distances along it).
         *
         * Each shape covers a segment of the line through the contact along
         * the normal; the middle of the part both cover is a point inside both
         * shapes, however deep one is in the other.
         */
        [[nodiscard]] constexpr float middleOfOverlap(float lowA, float highA, float lowB, float highB) noexcept {
            return (std::max(lowA, lowB) + std::min(highA, highB)) * 0.5f;
        }
    }

    /**
     * @brief Contact between two rectangles, or none when they do not overlap.
     *
     * Separates along the axis that needs the shortest push. On each axis
     * that push is the distance to the nearest side, not the width of the
     * overlap: a small rectangle inside a big one leaves through the
     * closest side. Ties go to Y, so that a rectangle landing exactly on a
     * corner is caught by the floor rather than by a wall.
     */
    [[nodiscard]] constexpr std::optional<Contact> collide(const Rectangle& a, const Rectangle& b) noexcept {
        // How far b must move to leave a, in each of the four directions.
        const float pushRight = a.max.x - b.min.x;
        const float pushLeft = b.max.x - a.min.x;
        const float pushUp = a.max.y - b.min.y;
        const float pushDown = b.max.y - a.min.y;
        if (pushRight < 0.f || pushLeft < 0.f || pushUp < 0.f || pushDown < 0.f) {
            return std::nullopt;
        }

        const Vector2f point{
            (std::max(a.min.x, b.min.x) + std::min(a.max.x, b.max.x)) * 0.5f,
            (std::max(a.min.y, b.min.y) + std::min(a.max.y, b.max.y)) * 0.5f};
        const float depthX = std::min(pushRight, pushLeft);
        const float depthY = std::min(pushUp, pushDown);
        if (depthX < depthY) {
            return Contact{{pushRight <= pushLeft ? 1.f : -1.f, 0.f}, depthX, point};
        }
        return Contact{{0.f, pushUp <= pushDown ? 1.f : -1.f}, depthY, point};
    }

    /**
     * @brief Contact between two circles, or none when they do not overlap.
     *
     * Concentric circles have no direction between them: they are pushed
     * apart along +Y, an arbitrary but fixed choice.
     */
    [[nodiscard]] inline std::optional<Contact> collide(const Circle& a, const Circle& b) noexcept {
        const Vector2f delta = b.center - a.center;
        const float radii = a.radius + b.radius;
        const float distanceSquared = delta.lengthSquared();
        if (distanceSquared > radii * radii) {
            return std::nullopt;
        }

        const float distance = std::sqrt(distanceSquared);
        // Divided per coordinate, not with Vector2's / (a multiply by 1 / length, off by
        // a float step): on a side, (0, d) / d must be exactly (0, 1).
        const Vector2f normal = distance > 0.f ? Vector2f(delta.x / distance, delta.y / distance) : Vector2f::unitY();
        // Along the normal from a's center, a covers [-ra, ra] and b covers [d - rb, d + rb].
        const float middle = Detail::middleOfOverlap(-a.radius, a.radius, distance - b.radius, distance + b.radius);
        return Contact{normal, radii - distance, a.center + normal * middle};
    }

    /**
     * @brief Contact between a rectangle and a circle, or none when they do not overlap.
     *
     * When the circle's center is outside the rectangle, the normal goes
     * from the rectangle's closest point to that center, which handles
     * corners exactly. When it is inside, the circle leaves through the
     * nearest side (ties go to Y, top first, as for rectangles).
     */
    [[nodiscard]] inline std::optional<Contact> collide(const Rectangle& rectangle, const Circle& circle) noexcept {
        const Vector2f closest = rectangle.closestPoint(circle.center);
        const Vector2f delta = circle.center - closest;
        const float distanceSquared = delta.lengthSquared();
        if (distanceSquared > circle.radius * circle.radius) {
            return std::nullopt;
        }

        const Vector2f size = rectangle.size();
        const float radius = circle.radius;

        if (distanceSquared > 0.f) {
            const float distance = std::sqrt(distanceSquared);
            const Vector2f normal{delta.x / distance, delta.y / distance};  // per coordinate: exact on a side
            // Along the normal from the circle's center, the circle covers [-r, r] and the
            // rectangle [-d - across, -d]: from closest, across it to the opposite side.
            // On a side, normal is (±1, 0) or (0, ±1) exactly; on a corner, both are non-zero.
            constexpr float Infinity = std::numeric_limits<float>::infinity();
            const float across = std::min(normal.x != 0.f ? size.x / std::abs(normal.x) : Infinity,
                                          normal.y != 0.f ? size.y / std::abs(normal.y) : Infinity);
            const float middle = Detail::middleOfOverlap(-radius, radius, -distance - across, -distance);
            return Contact{normal, radius - distance, circle.center + normal * middle};
        }

        // The center is inside the rectangle (or on its edge): leave through the nearest side.
        Vector2f normal{0.f, 1.f};
        float toSide = rectangle.max.y - circle.center.y;
        float across = size.y;
        if (const float toBottom = circle.center.y - rectangle.min.y; toBottom < toSide) {
            normal = {0.f, -1.f};
            toSide = toBottom;
        }
        if (const float toRight = rectangle.max.x - circle.center.x; toRight < toSide) {
            normal = {1.f, 0.f};
            toSide = toRight;
            across = size.x;
        }
        if (const float toLeft = circle.center.x - rectangle.min.x; toLeft < toSide) {
            normal = {-1.f, 0.f};
            toSide = toLeft;
            across = size.x;
        }
        // Along the normal from the center, the circle covers [-r, r] and the rectangle
        // [toSide - across, toSide].
        const float middle = Detail::middleOfOverlap(-radius, radius, toSide - across, toSide);
        return Contact{normal, radius + toSide, circle.center + normal * middle};
    }

    /** @brief Contact between a circle and a rectangle: collide(rectangle, circle) with the normal turned around. */
    [[nodiscard]] inline std::optional<Contact> collide(const Circle& circle, const Rectangle& rectangle) noexcept {
        std::optional<Contact> contact = collide(rectangle, circle);
        if (contact) {
            contact->normal = -contact->normal;
        }
        return contact;
    }
}
