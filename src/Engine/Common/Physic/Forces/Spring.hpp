#pragma once

/**
 * @file
 * @brief Damped spring (Hooke's law): F = -k * x - c * v.
 *
 * A spring links a body to an anchor and pulls it back towards its rest
 * length: grappling hooks and ropes, bouncy platforms, swinging objects,
 * a camera that follows smoothly.
 *
 * - k (stiffness, N/m): how hard the spring pulls per meter of stretch.
 * - c (damping, N·s/m): how fast the oscillations die out. 0 oscillates
 *   forever; criticalDamping() stops as fast as possible without
 *   overshooting.
 *
 * Units are SI (kilograms, meters, seconds, newtons). Stiff springs need
 * small time steps: when the body jitters or explodes away, lower k or the
 * step.
 */

#include <cmath>
#include <concepts>
#include <type_traits>

#include "../../Math/Vector.hpp"

namespace Engine::Physic {
    /**
     * @brief Force of a spring on the body at its end: -k * x along the spring.
     *
     * x is the stretch: current length minus rest length. A stretched
     * spring pulls the body towards the anchor; a compressed one pushes it
     * away.
     *
     * @param position Position of the body, in m.
     * @param anchor Position of the other end of the spring, in m.
     * @param restLength Length at which the spring exerts no force, in m, >= 0.
     * @param stiffness Stiffness k, in N/m, >= 0.
     * @return The force in N, or the zero vector when the body is at most
     *         Math::Epsilon from the anchor (no direction to push along).
     */
    template <std::floating_point T>
    [[nodiscard]] Math::Vector2<T> springForce(
        const Math::Vector2<T>& position, const Math::Vector2<T>& anchor,
        std::type_identity_t<T> restLength, std::type_identity_t<T> stiffness) noexcept {
        const Math::Vector2<T> fromAnchor = position - anchor;
        const T length = fromAnchor.length();
        if (length <= Math::Epsilon<T>) {
            return {};
        }
        const T stretch = length - restLength;
        return fromAnchor * (-stiffness * stretch / length);
    }

    /**
     * @brief Force of a damped spring: -k * x - c * v, both along the spring.
     *
     * Only the speed along the spring is damped, so a body swinging on a
     * rope keeps its sideways motion.
     *
     * A spring of rest length 0 has no line to act along: it is a joint
     * holding two points together (links of a chain joined end to end).
     * Its force is -k * d - c * v with the whole offset d and relative
     * velocity v: the joint is damped in every direction, and it still
     * acts when the two points meet, where the direction of the spring is
     * undefined. Damped along its line only, a joint would keep its
     * sideways jitter and pump energy into a chain.
     *
     * @param position Position of the body, in m.
     * @param anchor Position of the other end of the spring, in m.
     * @param relativeVelocity Velocity of the body minus that of the
     *        anchor, in m/s.
     * @param restLength Length at which the spring exerts no force, in m, >= 0.
     * @param stiffness Stiffness k, in N/m, >= 0.
     * @param damping Damping c, in N·s/m, >= 0.
     * @return The force in N, or the zero vector when the body is at most
     *         Math::Epsilon from the anchor (and the rest length is not 0).
     */
    template <std::floating_point T>
    [[nodiscard]] Math::Vector2<T> dampedSpringForce(
        const Math::Vector2<T>& position, const Math::Vector2<T>& anchor,
        const Math::Vector2<T>& relativeVelocity, std::type_identity_t<T> restLength,
        std::type_identity_t<T> stiffness, std::type_identity_t<T> damping) noexcept {
        const Math::Vector2<T> fromAnchor = position - anchor;
        if (restLength <= T{0}) {
            return fromAnchor * -stiffness - relativeVelocity * damping;  // a joint: every direction
        }
        const T length = fromAnchor.length();
        if (length <= Math::Epsilon<T>) {
            return {};
        }
        const Math::Vector2<T> direction = fromAnchor / length;
        const T stretch = length - restLength;
        const T stretchSpeed = relativeVelocity.dot(direction);
        return direction * (-stiffness * stretch - damping * stretchSpeed);
    }

    /**
     * @brief Damping that stops a spring as fast as possible without
     *        overshooting: c = 2 * sqrt(k * m).
     *
     * Below it, the body oscillates before settling; above it, it settles
     * more slowly.
     *
     * @param stiffness Stiffness k, in N/m, >= 0.
     * @param mass Mass of the body, in kg, >= 0.
     * @return The damping in N·s/m.
     */
    template <std::floating_point T>
    [[nodiscard]] T criticalDamping(T stiffness, std::type_identity_t<T> mass) noexcept {
        return T{2} * std::sqrt(stiffness * mass);
    }
}
