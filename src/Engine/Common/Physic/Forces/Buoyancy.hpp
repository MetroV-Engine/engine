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
 */

#include <algorithm>
#include <cmath>
#include <concepts>
#include <numbers>
#include <type_traits>

#include "../../Math/Vector.hpp"

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
}