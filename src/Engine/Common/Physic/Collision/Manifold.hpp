#pragma once

/**
 * @file
 * @brief Contact points: where two touching shapes press on each other,
 *        one or two points, for a response that turns bodies.
 *
 * collide() gives one point, the middle of the overlap: enough to push
 * bodies apart, not to turn them right. A box lying on the ground touches
 * it along a whole side; with a single point under its center, nothing
 * holds it level when something tips it (the friction of sliding, a load
 * on one end), and it rocks. Two points, at the ends of the touching
 * side, let the response press harder on one end than the other, as the
 * ground does.
 *
 * For two polygons (and rectangles): the "reference" face is the edge of
 * one most facing the normal, the "incident" face the edge of the other
 * most facing back. The incident edge is clipped to the reference face's
 * width; the ends left at or under the reference face are the contact
 * points, each with its own depth (Box2D's method). A curve touches at one
 * point: collide()'s.
 */

#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>

#include "Contact.hpp"
#include "Overlap.hpp"
#include "Shapes.hpp"

namespace Engine::Collision {
    /** @brief One point where two shapes press on each other. */
    struct ManifoldPoint {
        /** @brief Where, in world space: between the two surfaces. */
        Vector2f point;

        /** @brief How deep the shapes overlap there along the contact normal, in m, >= 0. */
        float depth = 0.f;
    };

    /** @brief The points where two shapes touch, along Contact::normal: one or two. */
    struct Manifold {
        std::array<ManifoldPoint, 2> points{};
        std::size_t count = 0;

        /** @brief Average of the points: the center of the contact. */
        [[nodiscard]] constexpr Vector2f center() const noexcept {
            return count == 2 ? (points[0].point + points[1].point) * 0.5f : points[0].point;
        }
    };

    namespace Detail {
        /** @brief The edge of the polygon whose outward normal is most along direction, and how much (the dot product). */
        [[nodiscard]] inline std::size_t faceAlong(const Polygon& polygon, const Vector2f& direction, float& alignment) noexcept {
            std::size_t best = 0;
            alignment = -2.f;
            for (std::size_t i = 0; i < polygon.count; ++i) {
                Vector2f normal;
                if (edgeNormal(polygon, i, normal)) {
                    if (const float along = normal.dot(direction); along > alignment) {
                        alignment = along;
                        best = i;
                    }
                }
            }
            return best;
        }
    }

    /** @brief The contact as one point: what collide() found. For shapes with a curve. */
    [[nodiscard]] constexpr Manifold singlePoint(const Contact& contact) noexcept {
        Manifold manifold;
        manifold.points[0] = {contact.point, contact.depth};
        manifold.count = 1;
        return manifold;
    }

    /**
     * @brief Where two convex polygons touch, given their contact
     *        (collide(a, b)): the incident edge clipped to the reference
     *        face, one or two points. Falls back to the contact's point
     *        when rounding leaves none.
     *
     * Computed around a's first vertex, where float steps are small.
     */
    [[nodiscard]] inline Manifold contactPoints(const Polygon& a, const Polygon& b, const Contact& contact) noexcept {
        const Vector2f origin = a.vertices[0];
        const Polygon localA = a.translated(-origin);
        const Polygon localB = b.translated(-origin);
        const Vector2f& normal = contact.normal;

        // The reference face: a's edge facing b, or b's edge facing a, whichever faces
        // the normal better (a's on a near tie, so the choice does not flicker).
        float alongA = 0.f;
        float alongB = 0.f;
        const std::size_t faceA = Detail::faceAlong(localA, normal, alongA);
        const std::size_t faceB = Detail::faceAlong(localB, -normal, alongB);
        const bool referenceIsB = alongB > alongA + 1e-6f;
        const Polygon& reference = referenceIsB ? localB : localA;
        const Polygon& incident = referenceIsB ? localA : localB;
        const std::size_t face = referenceIsB ? faceB : faceA;
        const Vector2f faceStart = reference.vertex(face);
        const Vector2f faceEnd = reference.vertex(face + 1);
        Vector2f faceNormal;
        if (!Detail::edgeNormal(reference, face, faceNormal)) {
            return singlePoint(contact);
        }

        // The incident edge: the one facing back at the reference face.
        float unused = 0.f;
        const std::size_t edge = Detail::faceAlong(incident, -faceNormal, unused);
        std::array<Vector2f, 2> ends{incident.vertex(edge), incident.vertex(edge + 1)};

        // Clip it to the reference face's width, between its two ends.
        const Vector2f along = (faceEnd - faceStart).normalized();
        const float low = along.dot(faceStart);
        const float high = along.dot(faceEnd);
        for (const float bound : {low, high}) {
            const float sign = bound == low ? 1.f : -1.f;  // keep along.p >= low, along.p <= high
            const float sideStart = sign * (along.dot(ends[0]) - bound);
            const float sideEnd = sign * (along.dot(ends[1]) - bound);
            if (sideStart < 0.f && sideEnd < 0.f) {
                return singlePoint(contact);
            }
            if (sideStart < 0.f) {
                ends[0] = ends[0] + (ends[1] - ends[0]) * (sideStart / (sideStart - sideEnd));
            } else if (sideEnd < 0.f) {
                ends[1] = ends[1] + (ends[0] - ends[1]) * (sideEnd / (sideEnd - sideStart));
            }
        }

        // Keep the ends at or under the reference face, halfway between the two surfaces.
        Manifold manifold;
        const float tolerance = 1e-4f + contact.depth * 1e-3f;
        for (const Vector2f& end : ends) {
            const float separation = faceNormal.dot(end - faceStart);
            if (separation <= tolerance && manifold.count < 2) {
                manifold.points[manifold.count++] = {end - faceNormal * (separation * 0.5f) + origin, std::max(0.f, -separation)};
            }
        }
        if (manifold.count == 0) {
            return singlePoint(contact);
        }
        if (manifold.count == 2 && (manifold.points[1].point - manifold.points[0].point).lengthSquared() <= 1e-12f) {
            manifold.count = 1;  // both ends at the same place: a corner on a face
        }
        return manifold;
    }

    /** @brief Where a rectangle and a polygon touch: the rectangle as a polygon. */
    [[nodiscard]] inline Manifold contactPoints(const Rectangle& a, const Polygon& b, const Contact& contact) noexcept {
        return contactPoints(Polygon::fromRectangle(a), b, contact);
    }

    /** @brief Where a polygon and a rectangle touch: the rectangle as a polygon. */
    [[nodiscard]] inline Manifold contactPoints(const Polygon& a, const Rectangle& b, const Contact& contact) noexcept {
        return contactPoints(a, Polygon::fromRectangle(b), contact);
    }

    /** @brief Where two rectangles touch: as polygons. */
    [[nodiscard]] inline Manifold contactPoints(const Rectangle& a, const Rectangle& b, const Contact& contact) noexcept {
        return contactPoints(Polygon::fromRectangle(a), Polygon::fromRectangle(b), contact);
    }

    /** @brief Where a circle touches anything, or anything a circle: at one point, collide()'s. */
    template <typename A, typename B>
        requires(std::same_as<A, Circle> || std::same_as<B, Circle>)
    [[nodiscard]] constexpr Manifold contactPoints(const A&, const B&, const Contact& contact) noexcept {
        return singlePoint(contact);
    }
}
