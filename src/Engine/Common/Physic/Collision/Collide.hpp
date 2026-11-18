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
#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
#include <optional>

#include "Contact.hpp"
#include "Overlap.hpp"
#include "Shapes.hpp"

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

        /** @brief A way to push the second shape out of the first: a unit direction and a distance. */
        struct Push {
            Vector2f normal;
            float depth = std::numeric_limits<float>::infinity();
        };

        /**
         * @brief Whether push is a better choice than best: shorter, or as
         *        short and closer to vertical (ties go to Y), or as vertical
         *        and pointing up or right. The same choices as
         *        collide(Rectangle, Rectangle), so a rectangle given as a
         *        polygon gets the same contact.
         */
        [[nodiscard]] constexpr bool isBetter(const Push& push, const Push& best) noexcept {
            if (push.depth != best.depth) {
                return push.depth < best.depth;
            }
            const float vertical = push.normal.y < 0.f ? -push.normal.y : push.normal.y;
            const float bestVertical = best.normal.y < 0.f ? -best.normal.y : best.normal.y;
            if (vertical != bestVertical) {
                return vertical > bestVertical;
            }
            return push.normal.x + push.normal.y > best.normal.x + best.normal.y;
        }

        /**
         * @brief Along each edge normal of `edges`, the two pushes that take
         *        b out of a (along it and against it); keeps the best in
         *        best. The caller has checked that they overlap: a push a
         *        rounding below 0 (shapes touching on a slanted edge) is 0.
         */
        inline void shortestPush(const Polygon& edges, const Polygon& a, const Polygon& b, Push& best) noexcept {
            for (std::size_t i = 0; i < edges.count; ++i) {
                Vector2f normal;
                if (!edgeNormal(edges, i, normal)) {
                    continue;
                }
                const Interval spanA = project(a, normal);
                const Interval spanB = project(b, normal);
                const Push along{normal, std::max(0.f, spanA.max - spanB.min)};      // b moves along the normal
                const Push against{-normal, std::max(0.f, spanB.max - spanA.min)};   // b moves against it
                if (isBetter(along, best)) {
                    best = along;
                }
                if (isBetter(against, best)) {
                    best = against;
                }
            }
        }

        /** @brief Whether one of the polygon's edges has length 0: a Rectangle of size 0 given as a polygon. */
        [[nodiscard]] constexpr bool hasEmptyEdge(const Polygon& polygon) noexcept {
            for (std::size_t i = 0; i < polygon.count; ++i) {
                if (const Vector2f along = polygon.edge(i); along.x == 0.f && along.y == 0.f) {
                    return true;
                }
            }
            return false;
        }

        /**
         * @brief A point inside both polygons: the average of the vertices of
         *        their intersection, b clipped by every edge of a
         *        (Sutherland-Hodgman). The intersection of two convex
         *        polygons is convex, so the average of its vertices is in it.
         *        When rounding leaves nothing (they only touch), the point of
         *        a closest to b's deepest vertex along -normal.
         *
         *        An edge of length 0 bounds nothing, so a rectangle of size 0
         *        (a segment) cannot clip: then it is the one clipped.
         */
        [[nodiscard]] inline Vector2f middleOfIntersection(const Polygon& a, const Polygon& b, const Vector2f& normal) noexcept {
            if (hasEmptyEdge(a) && !hasEmptyEdge(b)) {
                return middleOfIntersection(b, a, -normal);
            }
            // Clipping by one edge adds at most one vertex.
            constexpr std::size_t Capacity = 2 * Polygon::MaxVertices;
            std::array<Vector2f, Capacity> current{};
            std::array<Vector2f, Capacity> next{};
            std::size_t currentCount = b.count;
            std::copy(b.vertices.begin(), b.vertices.begin() + static_cast<std::ptrdiff_t>(b.count), current.begin());
            for (std::size_t i = 0; i < a.count && currentCount > 0; ++i) {
                const Vector2f start = a.vertex(i);
                const Vector2f along = a.edge(i);
                // >= 0: on the inner side of edge i (its left).
                const auto side = [&](const Vector2f& point) { return along.cross(point - start); };
                std::size_t nextCount = 0;
                for (std::size_t j = 0; j < currentCount; ++j) {
                    const Vector2f& from = current[j];
                    const Vector2f& to = current[(j + 1) % currentCount];
                    const float sideFrom = side(from);
                    const float sideTo = side(to);
                    if (sideFrom >= 0.f) {
                        next[nextCount++] = from;
                    }
                    if ((sideFrom >= 0.f) != (sideTo >= 0.f)) {
                        next[nextCount++] = from + (to - from) * (sideFrom / (sideFrom - sideTo));
                    }
                }
                current.swap(next);
                currentCount = std::min(nextCount, Capacity);
            }
            if (currentCount == 0) {
                return a.closestPoint(b.support(-normal));
            }
            Vector2f sum;
            for (std::size_t j = 0; j < currentCount; ++j) {
                sum += current[j];
            }
            return sum / static_cast<float>(currentCount);
        }

        /**
         * @brief How far a unit direction goes from start, a point in or on
         *        the polygon, before leaving it: the nearest edge line it
         *        crosses going out.
         */
        [[nodiscard]] inline float exitDistance(const Polygon& polygon, const Vector2f& start, const Vector2f& direction) noexcept {
            float exit = std::numeric_limits<float>::infinity();
            for (std::size_t i = 0; i < polygon.count; ++i) {
                const Vector2f normal = polygon.normal(i);
                if (const float facing = normal.dot(direction); facing > 0.f) {
                    exit = std::min(exit, std::max(0.f, normal.dot(polygon.vertex(i) - start) / facing));
                }
            }
            return exit;
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

    /**
     * @brief Contact between two convex polygons, or none when they do not
     *        overlap.
     *
     * Separating axis theorem: along each edge normal of either polygon,
     * their spans must meet, and the shortest push that separates them is
     * along one of those normals. Ties go to Y as for rectangles, so a
     * rectangle given as a polygon gets the same contact as
     * collide(Rectangle, Rectangle). The point is the middle of their
     * intersection.
     *
     * Computed around a's first vertex: subtracting nearby coordinates is
     * exact, and projections then stay small, where float steps are tiny.
     */
    [[nodiscard]] inline std::optional<Contact> collide(const Polygon& a, const Polygon& b) noexcept {
        // Whether they meet is overlaps()'s answer: its axes are not normalized, so it is
        // exact on grid coordinates, and collide() always agrees with it.
        if (!overlaps(a, b)) {
            return std::nullopt;
        }
        const Vector2f origin = a.vertices[0];
        const Polygon localA = a.translated(-origin);
        const Polygon localB = b.translated(-origin);
        Detail::Push best;
        Detail::shortestPush(localA, localA, localB, best);
        Detail::shortestPush(localB, localA, localB, best);
        const Vector2f point = Detail::middleOfIntersection(localA, localB, best.normal) + origin;
        return Contact{best.normal, best.depth, point};
    }

    /** @brief Contact between a polygon and a rectangle: the rectangle as a polygon. */
    [[nodiscard]] inline std::optional<Contact> collide(const Polygon& polygon, const Rectangle& rectangle) noexcept {
        return collide(polygon, Polygon::fromRectangle(rectangle));
    }

    /** @brief Contact between a rectangle and a polygon: the rectangle as a polygon. */
    [[nodiscard]] inline std::optional<Contact> collide(const Rectangle& rectangle, const Polygon& polygon) noexcept {
        return collide(Polygon::fromRectangle(rectangle), polygon);
    }

    namespace Detail {
        /** @brief collide(polygon, circle) for a circle of this radius centered at (0, 0). */
        [[nodiscard]] inline std::optional<Contact> collideAroundCenter(const Polygon& polygon, float radius) noexcept {
            const Vector2f center = Vector2f::zero();
            const Vector2f closest = polygon.closestPoint(center);
            const Vector2f delta = center - closest;
            const float distanceSquared = delta.lengthSquared();
            if (distanceSquared > radius * radius) {
                return std::nullopt;
            }

            if (distanceSquared > 0.f) {
                const float distance = std::sqrt(distanceSquared);
                const Vector2f normal{delta.x / distance, delta.y / distance};  // per coordinate: exact on an axis
                // Along the normal from the circle's center, the circle covers [-r, r] and the
                // polygon [-d - across, -d]: from closest, across it to where it leaves.
                const float across = exitDistance(polygon, closest, -normal);
                const float middle = middleOfOverlap(-radius, radius, -distance - across, -distance);
                return Contact{normal, radius - distance, center + normal * middle};
            }

            // The center is inside the polygon (or on an edge): leave through the nearest edge.
            Push best;
            for (std::size_t i = 0; i < polygon.count; ++i) {
                const Vector2f normal = polygon.normal(i);
                const Push out{normal, normal.dot(polygon.vertex(i) - center)};
                if (isBetter(out, best)) {
                    best = out;
                }
            }
            // Along the normal from the center, the circle covers [-r, r] and the polygon
            // [-back, toSide].
            const float toSide = best.depth;
            const float back = exitDistance(polygon, center, -best.normal);
            const float middle = middleOfOverlap(-radius, radius, -back, toSide);
            return Contact{best.normal, radius + toSide, center + best.normal * middle};
        }
    }

    /**
     * @brief Contact between a polygon and a circle, or none when they do
     *        not overlap.
     *
     * As for a rectangle: when the circle's center is outside, the normal
     * goes from the polygon's closest point to the center, which handles
     * vertices exactly; when it is inside, the circle leaves through the
     * nearest edge (ties go to Y, top first).
     *
     * Computed around the circle's center: far from the origin, a closest
     * point on a slanted edge is only known to a float step of the
     * coordinates, and a center that close would get a wrong normal.
     */
    [[nodiscard]] inline std::optional<Contact> collide(const Polygon& polygon, const Circle& circle) noexcept {
        const std::optional<Contact> local = Detail::collideAroundCenter(polygon.translated(-circle.center), circle.radius);
        if (!local) {
            return std::nullopt;
        }
        return Contact{local->normal, local->depth, local->point + circle.center};
    }

    /** @brief Contact between a circle and a polygon: collide(polygon, circle) with the normal turned around. */
    [[nodiscard]] inline std::optional<Contact> collide(const Circle& circle, const Polygon& polygon) noexcept {
        std::optional<Contact> contact = collide(polygon, circle);
        if (contact) {
            contact->normal = -contact->normal;
        }
        return contact;
    }
}
