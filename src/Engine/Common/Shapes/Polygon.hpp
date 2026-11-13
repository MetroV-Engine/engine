#pragma once

/**
 * @file
 * @brief Polygon: a convex shape with straight edges, such as a slope.
 */

#include <algorithm>
#include <array>
#include <cstddef>
#include <initializer_list>
#include <limits>

#include "../Math/Vector.hpp"
#include "Rectangle.hpp"

namespace Engine::Shapes {
    /**
     * @brief Convex polygon of 3 to MaxVertices vertices, counter-clockwise,
     *        in world space and meters.
     *
     * What rectangles and circles cannot be: a slope (a right triangle), a
     * ramp, a rotated box, a cut corner. Like the other shapes it never
     * allocates: the vertices live in a fixed array, so a Polygon copies
     * like a plain value and fits in a component.
     *
     * Invariant, checked by isConvex(): at least 3 vertices, given
     * counter-clockwise (the inside on the left of each edge), every vertex
     * strictly on the inside of every other edge: strictly convex, no three
     * vertices aligned. Every query relies on it. Edge i goes from
     * vertex(i) to vertex(i + 1), the last one back to the first.
     */
    struct Polygon {
        /** @brief Most vertices a polygon can have: enough for slopes, ramps and rounded-off boxes. */
        static constexpr std::size_t MaxVertices = 8;

        /** @brief The vertices; only the first `count` are used. */
        std::array<Vector2f, MaxVertices> vertices{};

        /** @brief Number of vertices, in [3, MaxVertices] for a valid polygon. */
        std::size_t count = 0;

        constexpr Polygon() noexcept = default;

        /** @brief The polygon of these vertices, counter-clockwise; past MaxVertices they are ignored. */
        constexpr Polygon(std::initializer_list<Vector2f> points) noexcept {
            for (const Vector2f& point : points) {
                if (count == MaxVertices) {
                    break;
                }
                vertices[count++] = point;
            }
        }

        /** @brief The rectangle as a polygon: min, then counter-clockwise. */
        [[nodiscard]] static constexpr Polygon fromRectangle(const Rectangle& rectangle) noexcept {
            return {rectangle.min, {rectangle.max.x, rectangle.min.y}, rectangle.max, {rectangle.min.x, rectangle.max.y}};
        }

        /** @brief Vertex i, for i in [0, count): i = count wraps to the first one. */
        [[nodiscard]] constexpr const Vector2f& vertex(std::size_t i) const noexcept {
            return vertices[i == count ? 0 : i];
        }

        /** @brief Edge i, from vertex(i) to vertex(i + 1). */
        [[nodiscard]] constexpr Vector2f edge(std::size_t i) const noexcept { return vertex(i + 1) - vertex(i); }

        /**
         * @brief Unit normal of edge i, pointing out of the polygon: the edge
         *        turned clockwise, (edge.y, -edge.x) / length.
         *
         * Divided per coordinate, not with Vector2's / (a multiply by
         * 1 / length, off by a float step): an axis-aligned edge has
         * exactly (1, 0), (0, 1), (-1, 0) or (0, -1), as a Rectangle's sides.
         */
        [[nodiscard]] Vector2f normal(std::size_t i) const noexcept {
            const Vector2f along = edge(i);
            const float length = along.length();
            return {along.y / length, -along.x / length};
        }

        /** @brief Average of the vertices: inside a convex polygon. */
        [[nodiscard]] constexpr Vector2f center() const noexcept {
            Vector2f sum;
            for (std::size_t i = 0; i < count; ++i) {
                sum += vertices[i];
            }
            return sum / static_cast<float>(count);
        }

        /** @brief Area enclosed, in m² (shoelace formula): > 0 for a valid, counter-clockwise polygon. */
        [[nodiscard]] constexpr float area() const noexcept {
            float twice = 0.f;
            for (std::size_t i = 0; i < count; ++i) {
                twice += vertices[i].cross(vertex(i + 1));
            }
            return twice / 2.f;
        }

        /**
         * @brief Center of the enclosed area (where a uniform plate
         *        balances): the triangles from the first vertex, averaged by
         *        area. Differs from center() unless the polygon is regular.
         *        Computed around the first vertex, where float steps are small.
         */
        [[nodiscard]] constexpr Vector2f centroid() const noexcept {
            const Vector2f origin = vertices[0];
            Vector2f weighted;
            float twiceArea = 0.f;
            for (std::size_t i = 1; i + 1 < count; ++i) {
                const Vector2f b = vertices[i] - origin;
                const Vector2f c = vertices[i + 1] - origin;
                const float twice = b.cross(c);
                weighted += (b + c) * twice;  // triangle (0, b, c): centroid (b + c) / 3, area twice / 2
                twiceArea += twice;
            }
            return origin + weighted / (3.f * twiceArea);
        }

        /**
         * @brief The polygon turned by radians around (0, 0): its vertices
         *        given relative to a center, turned with the body there.
         *        Still counter-clockwise and convex.
         */
        [[nodiscard]] Polygon rotated(float radians) const noexcept {
            Polygon turned = *this;
            for (std::size_t i = 0; i < count; ++i) {
                turned.vertices[i] = vertices[i].rotated(radians);
            }
            return turned;
        }

        /** @brief The smallest axis-aligned rectangle containing the polygon. */
        [[nodiscard]] constexpr Rectangle boundingBox() const noexcept {
            Rectangle box{vertices[0], vertices[0]};
            for (std::size_t i = 1; i < count; ++i) {
                box.min = Math::min(box.min, vertices[i]);
                box.max = Math::max(box.max, vertices[i]);
            }
            return box;
        }

        /** @brief The same polygon moved by offset. */
        [[nodiscard]] constexpr Polygon translated(const Vector2f& offset) const noexcept {
            Polygon moved = *this;
            for (std::size_t i = 0; i < count; ++i) {
                moved.vertices[i] += offset;
            }
            return moved;
        }

        /** @brief The vertex furthest along direction (the first one on a tie). */
        [[nodiscard]] constexpr const Vector2f& support(const Vector2f& direction) const noexcept {
            std::size_t best = 0;
            float furthest = vertices[0].dot(direction);
            for (std::size_t i = 1; i < count; ++i) {
                if (const float along = vertices[i].dot(direction); along > furthest) {
                    furthest = along;
                    best = i;
                }
            }
            return vertices[best];
        }

        /** @brief Whether point is inside or on the edges: on the left of, or on, every edge. */
        [[nodiscard]] constexpr bool contains(const Vector2f& point) const noexcept {
            for (std::size_t i = 0; i < count; ++i) {
                if (edge(i).cross(point - vertices[i]) < 0.f) {
                    return false;
                }
            }
            return true;
        }

        /** @brief The point of the polygon closest to point: point itself when it is inside. */
        [[nodiscard]] constexpr Vector2f closestPoint(const Vector2f& point) const noexcept {
            if (contains(point)) {
                return point;
            }
            Vector2f closest = vertices[0];
            float closestSquared = std::numeric_limits<float>::infinity();
            for (std::size_t i = 0; i < count; ++i) {
                // Closest point of the segment: the projection on its line, clamped to its ends.
                const Vector2f along = edge(i);
                const Vector2f fromStart = point - vertices[i];
                const float share = std::clamp(fromStart.dot(along) / along.lengthSquared(), 0.f, 1.f);
                const Vector2f onEdge = vertices[i] + along * share;
                if (const float distanceSquared = (point - onEdge).lengthSquared(); distanceSquared < closestSquared) {
                    closestSquared = distanceSquared;
                    closest = onEdge;
                }
            }
            return closest;
        }

        /**
         * @brief Whether the invariant holds: 3 to MaxVertices vertices,
         *        counter-clockwise, strictly convex.
         *
         * Every vertex must be strictly on the left of every edge it is not
         * an end of. That rules out clockwise order, aligned vertices,
         * repeated vertices and self-crossing stars alike.
         */
        [[nodiscard]] constexpr bool isConvex() const noexcept {
            if (count < 3 || count > MaxVertices) {
                return false;
            }
            for (std::size_t i = 0; i < count; ++i) {
                for (std::size_t j = 0; j < count; ++j) {
                    if (j != i && j != (i + 1) % count && edge(i).cross(vertices[j] - vertices[i]) <= 0.f) {
                        return false;
                    }
                }
            }
            return true;
        }

        /** @brief Same vertices, in the same order from the same first one. */
        [[nodiscard]] constexpr bool operator==(const Polygon& other) const noexcept {
            return count == other.count && std::equal(vertices.begin(), vertices.begin() + static_cast<std::ptrdiff_t>(count),
                                                      other.vertices.begin());
        }
    };
}
