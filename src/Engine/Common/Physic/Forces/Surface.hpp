#pragma once

/**
 * @file
 * @brief Splitting forces and velocities against a surface in contact.
 *
 * A vector splits into a normal part, perpendicular to the surface, and a
 * tangential part, along it. Against a slope, the weight's normal part
 * presses the body onto the surface and gives the normal force N used by
 * Friction.hpp; its tangential part pulls the body down the slope.
 *
 * Every surfaceNormal parameter is the unit normal of the surface (length
 * 1), pointing out of the surface, towards the body resting on it.
 */

#include <algorithm>
#include <concepts>
#include <type_traits>

#include "../../Math/Vector.hpp"

namespace Engine::Physic {
    /**
     * @brief Part of a vector perpendicular to the surface: (v . n) * n.
     * @param vector Force or velocity to split.
     * @param surfaceNormal Unit normal of the surface.
     */
    template <std::floating_point T>
    [[nodiscard]] constexpr Math::Vector2<T> normalComponent(
        const Math::Vector2<T>& vector, const Math::Vector2<T>& surfaceNormal) noexcept {
        return surfaceNormal * vector.dot(surfaceNormal);
    }

    /**
     * @brief Part of a vector along the surface: v - (v . n) * n.
     * @param vector Force or velocity to split.
     * @param surfaceNormal Unit normal of the surface.
     */
    template <std::floating_point T>
    [[nodiscard]] constexpr Math::Vector2<T> tangentialComponent(
        const Math::Vector2<T>& vector, const Math::Vector2<T>& surfaceNormal) noexcept {
        return vector - normalComponent(vector, surfaceNormal);
    }

    /**
     * @brief Normal force N: how hard a force presses a body onto a surface.
     *
     * The surface pushes back with the same strength, so this is the N of
     * Friction.hpp.
     *
     * @param force Sum of the forces pressing on the body (its weight, for
     *        a body resting on the surface), in N.
     * @param surfaceNormal Unit normal of the surface.
     * @return The strength in N, or 0 when the force pulls the body away
     *         from the surface (no contact force, so no friction).
     */
    template <std::floating_point T>
    [[nodiscard]] constexpr T normalForce(
        const Math::Vector2<T>& force, const Math::Vector2<T>& surfaceNormal) noexcept {
        return std::max(T{0}, -force.dot(surfaceNormal));
    }
}
