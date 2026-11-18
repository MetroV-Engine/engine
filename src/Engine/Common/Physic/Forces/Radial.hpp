#pragma once

/**
 * @file
 * @brief Radial push from a point, fading with distance: explosions,
 *        shockwaves, repulsion fields.
 *
 * The result points away from the center, so a negative strength pulls
 * towards it instead. Use it as a force, applied every step (a field), or
 * as an impulse, applied once (a blast): see Dynamics.hpp.
 */

#include <cmath>
#include <concepts>
#include <type_traits>

#include "../../Math/Vector.hpp"

namespace Engine::Physic {
    /**
     * @brief Radial push with linear falloff: full strength at the center,
     *        0 at the radius and beyond.
     *
     * Strength at distance d: strength * (1 - d / radius).
     *
     * @param target Position of the pushed body, in m.
     * @param center Center of the push, in m.
     * @param strength Strength at the center, in N for a force or N·s for
     *        an impulse.
     * @param radius Distance at which the push fades out, in m, > 0.
     * @return The push, or the zero vector when the target is at the
     *         radius or beyond, or at most Math::Epsilon from the center
     *         (no direction to push along).
     */
    template <std::floating_point T>
    [[nodiscard]] Math::Vector2<T> radialPush(
        const Math::Vector2<T>& target, const Math::Vector2<T>& center,
        std::type_identity_t<T> strength, std::type_identity_t<T> radius) noexcept {
        const Math::Vector2<T> fromCenter = target - center;
        const T distanceSquared = fromCenter.lengthSquared();
        // Out of range or at the center: rejected before the square root,
        // since most bodies of a scene are out of range of a given blast.
        if (distanceSquared >= radius * radius
            || distanceSquared <= Math::Epsilon<T> * Math::Epsilon<T>) {
            return {};
        }
        const T distance = std::sqrt(distanceSquared);
        const T falloff = T{1} - distance / radius;
        return fromCenter * (strength * falloff / distance);
    }
}
