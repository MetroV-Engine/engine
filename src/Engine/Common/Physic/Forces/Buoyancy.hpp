#pragma once

/**
 * @file
 * @brief Buoyancy (Archimedes): a fluid pushes a body up by the weight of the
 *        fluid it displaces.
 *
 * F = -rho * A * g, with rho the fluid's density, A the body's area under the
 * surface and g the gravity: opposite to gravity, and as strong as the weight
 * of the displaced fluid. A body floats when its own density (mass / area) is
 * below the fluid's: it sinks until the part under the surface weighs as much
 * as it does.
 *
 * In 2D, densities are per area (kg/m²) and areas in m², as if every body
 * were 1 m deep. Bodies of a game are much lighter per area than real ones
 * (a 70 kg character of 0.8 m by 1.8 m is 49 kg/m²): pick the fluid's density
 * from the bodies that should float, not from real water.
 *
 * submergedPart() measures any shape inside a fluid's box, below its top:
 * the area A, and its centroid, the "center of buoyancy" where the push
 * applies. Off the body's center of mass, the push turns the body: a
 * tilted raft rights itself.
 */

#include <algorithm>
#include <cmath>
#include <concepts>
#include <numbers>
#include <type_traits>

#include <array>
#include <cstddef>

#include "../../Math/Vector.hpp"
#include "../../Shapes/Circle.hpp"
#include "../../Shapes/Polygon.hpp"
#include "../../Shapes/Rectangle.hpp"

namespace Engine::Physic {
    /**
     * @brief Area of an axis-aligned box under a horizontal surface.
     * @param bottom Height of the box's bottom side, in m.
     * @param top Height of its top side, in m, >= bottom.
     * @param width Its width, in m.
     * @param surface Height of the fluid's surface, in m: the fluid is below.
     * @return The area under the surface, in m²: 0 above it, the whole box under it.
     */
    template <std::floating_point T>
    [[nodiscard]] constexpr T submergedBoxArea(
        T bottom, std::type_identity_t<T> top, std::type_identity_t<T> width, std::type_identity_t<T> surface) noexcept {
        return width * std::clamp(surface - bottom, T{0}, top - bottom);
    }

    /**
     * @brief Area of a disc under a horizontal surface: the circular segment
     *        of height h = surface - (center - radius).
     *
     * r² acos((r - h) / r) - (r - h) sqrt(2 r h - h²), from 0 (h = 0) to
     * pi r² (h = 2 r).
     *
     * @param centerHeight Height of the disc's center, in m.
     * @param radius Its radius, in m, >= 0.
     * @param surface Height of the fluid's surface, in m: the fluid is below.
     * @return The area under the surface, in m².
     */
    template <std::floating_point T>
    [[nodiscard]] T submergedCircleArea(
        T centerHeight, std::type_identity_t<T> radius, std::type_identity_t<T> surface) noexcept {
        const T h = std::clamp(surface - (centerHeight - radius), T{0}, T{2} * radius);
        if (h <= T{0}) {
            return T{0};
        }
        const T fromCenter = radius - h;  // signed: negative once more than half is under
        return radius * radius * std::acos(std::clamp(fromCenter / radius, T{-1}, T{1}))
             - fromCenter * std::sqrt(std::max(T{0}, T{2} * radius * h - h * h));
    }

    /**
     * @brief Buoyancy: the weight of the displaced fluid, opposite to gravity.
     * @param fluidDensity rho, in kg/m² (2D), >= 0.
     * @param submergedArea A, the body's area under the surface, in m².
     * @param gravity The gravity acceleration, in m/s².
     * @return The force in N: -rho * A * g.
     */
    template <std::floating_point T>
    [[nodiscard]] constexpr Math::Vector2<T> buoyancyForce(
        std::type_identity_t<T> fluidDensity, std::type_identity_t<T> submergedArea, const Math::Vector2<T>& gravity) noexcept {
        return gravity * -(fluidDensity * submergedArea);
    }

    /** @brief The part of a shape inside a fluid: its area, and where its center is. */
    struct SubmergedPart {
        /** @brief Area inside the fluid, in m²: 0 when out of it. */
        float area = 0.f;

        /** @brief Centroid of that area, in world space: where the buoyancy pushes. Meaningless when area is 0. */
        Math::Vector2f centroid;
    };

    namespace Detail {
        /** @brief A convex outline of up to Capacity points, for clipping: polygons and approximated circles. */
        struct Outline {
            static constexpr std::size_t Capacity = 48;
            std::array<Math::Vector2f, Capacity> points{};
            std::size_t count = 0;
        };

        /**
         * @brief The outline clipped to the inside of one axis-aligned bound
         *        (Sutherland-Hodgman): keeps the points with point[axis] *
         *        sign <= limit * sign.
         */
        [[nodiscard]] inline Outline clip(const Outline& outline, int axis, float limit, float sign) noexcept {
            Outline kept;
            const auto inside = [&](const Math::Vector2f& point) {
                return ((axis == 0 ? point.x : point.y) - limit) * sign <= 0.f;
            };
            for (std::size_t i = 0; i < outline.count; ++i) {
                const Math::Vector2f& from = outline.points[i];
                const Math::Vector2f& to = outline.points[(i + 1) % outline.count];
                const bool fromInside = inside(from);
                if (fromInside && kept.count < Outline::Capacity) {
                    kept.points[kept.count++] = from;
                }
                if (fromInside != inside(to) && kept.count < Outline::Capacity) {
                    const float a = axis == 0 ? from.x : from.y;
                    const float b = axis == 0 ? to.x : to.y;
                    kept.points[kept.count++] = from + (to - from) * ((limit - a) / (b - a));
                }
            }
            return kept;
        }

        /** @brief Area and centroid of the outline clipped to the box (shoelace), around its first point. */
        [[nodiscard]] inline SubmergedPart insideBox(Outline outline, const Shapes::Rectangle& box) noexcept {
            outline = clip(outline, 0, box.min.x, -1.f);
            outline = clip(outline, 0, box.max.x, 1.f);
            outline = clip(outline, 1, box.min.y, -1.f);
            outline = clip(outline, 1, box.max.y, 1.f);
            if (outline.count < 3) {
                return {};
            }
            const Math::Vector2f origin = outline.points[0];
            float twiceArea = 0.f;
            Math::Vector2f weighted;
            for (std::size_t i = 1; i + 1 < outline.count; ++i) {
                const Math::Vector2f b = outline.points[i] - origin;
                const Math::Vector2f c = outline.points[i + 1] - origin;
                const float twice = b.cross(c);
                twiceArea += twice;
                weighted += (b + c) * twice;
            }
            if (twiceArea <= 0.f) {
                return {};
            }
            return {twiceArea / 2.f, origin + weighted / (3.f * twiceArea)};
        }
    }

    /**
     * @brief The part of a convex polygon inside a fluid's box (a rectangle
     *        whose top is the surface): the polygon clipped to the box, its
     *        area and centroid exactly.
     */
    [[nodiscard]] inline SubmergedPart submergedPart(const Shapes::Polygon& shape, const Shapes::Rectangle& fluid) noexcept {
        Detail::Outline outline;
        for (std::size_t i = 0; i < shape.count; ++i) {
            outline.points[outline.count++] = shape.vertices[i];
        }
        return Detail::insideBox(outline, fluid);
    }

    /** @brief The part of a rectangle inside a fluid's box: exact. */
    [[nodiscard]] inline SubmergedPart submergedPart(const Shapes::Rectangle& shape, const Shapes::Rectangle& fluid) noexcept {
        return submergedPart(Shapes::Polygon::fromRectangle(shape), fluid);
    }

    /**
     * @brief The part of a circle inside a fluid's box.
     *
     * Exact when only the surface cuts it (the circle within the box's
     * sides and above its bottom, the usual case of a floating ball): the
     * circular segment under the surface, of area submergedCircleArea() and
     * centroid 2 (r² - d²)^(3/2) / (3 A) below the center, d the surface's
     * height above it. Crossing a side or the bottom, the circle is
     * approximated by a 32-gon of the same area, clipped like a polygon
     * (within 0.1 % of the area).
     */
    [[nodiscard]] inline SubmergedPart submergedPart(const Shapes::Circle& shape, const Shapes::Rectangle& fluid) noexcept {
        const float r = shape.radius;
        const Math::Vector2f& c = shape.center;
        if (r <= 0.f) {
            return {};
        }
        const bool withinSides = c.x - r >= fluid.min.x && c.x + r <= fluid.max.x;
        const bool aboveBottom = c.y - r >= fluid.min.y;
        if (withinSides && aboveBottom) {
            const float area = submergedCircleArea(c.y, r, fluid.max.y);
            if (area <= 0.f) {
                return {};
            }
            const float d = std::clamp(fluid.max.y - c.y, -r, r);
            const float chord = std::max(0.f, r * r - d * d);
            return {area, {c.x, c.y - 2.f * chord * std::sqrt(chord) / (3.f * area)}};
        }
        // A regular 32-gon of the circle's area: its circumradius is r * sqrt(pi / (n/2 sin(2 pi / n))).
        constexpr std::size_t Sides = 32;
        const float tau = 2.f * std::numbers::pi_v<float>;
        const float scale = std::sqrt(std::numbers::pi_v<float> / (0.5f * Sides * std::sin(tau / Sides)));
        Detail::Outline outline;
        for (std::size_t i = 0; i < Sides; ++i) {
            const float angle = tau * static_cast<float>(i) / Sides;
            outline.points[outline.count++] = c + Math::Vector2f(std::cos(angle), std::sin(angle)) * (r * scale);
        }
        return Detail::insideBox(outline, fluid);
    }
}
