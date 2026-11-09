#pragma once

/**
 * @file
 * @brief Gravity: uniform (weight, F = m * g) and universal (Newton's law,
 *        F = G * (mA * mB) / d²).
 *
 * Units are SI (kilograms, meters, newtons, m/s²).
 *
 * Uniform gravity is the weight of bodies near the ground of a planet: the
 * same acceleration g everywhere, which is what a platformer's characters
 * fall with. The gravity vector is a parameter, so the direction of "down"
 * stays the caller's choice (0, -g) when Y points up, (0, g) when Y points
 * down.
 *
 * Universal gravitation is the attraction between two bodies, for planets
 * or attractors. The real G is tiny, so masses and distances at game scale
 * produce forces too small to notice: pass a larger gravitationalConstant
 * to tune the gravity of a scene. Two bodies closer than Math::Epsilon
 * attract with a force of 0: the formula diverges when d reaches 0, and
 * such bodies have no reliable direction between them.
 */

#include <cmath>
#include <concepts>
#include <type_traits>

#include "../../Math/Vector.hpp"

namespace Engine::Physic {
    /** @brief Standard acceleration of gravity at the Earth's surface, in m/s². */
    template <std::floating_point T>
    inline constexpr T StandardGravity = static_cast<T>(9.80665);

    /**
     * @brief Weight of a body under uniform gravity: m * g * gravityScale.
     * @param mass Mass of the body, in kg.
     * @param gravity Acceleration of gravity, in m/s², pointing "down".
     * @param gravityScale Multiplier for this body: 0 ignores gravity, above
     *        1 falls faster (e.g. a snappier fall after a jump's apex).
     * @return The force in N.
     */
    template <std::floating_point T>
    [[nodiscard]] constexpr Math::Vector2<T> weight(
        std::type_identity_t<T> mass, const Math::Vector2<T>& gravity,
        std::type_identity_t<T> gravityScale = T{1}) noexcept {
        return gravity * (mass * gravityScale);
    }

    /** @brief Gravitational constant G, in N·m²/kg² (CODATA 2018). */
    template <std::floating_point T>
    inline constexpr T GravitationalConstant = static_cast<T>(6.67430e-11);

    /**
     * @brief Strength of the attraction between two masses: G * mA * mB / d².
     * @param massA Mass of the first body, in kg.
     * @param massB Mass of the second body, in kg.
     * @param distance Distance between the centers of the bodies, in m.
     * @param gravitationalConstant G to use; the real one by default.
     * @return The force in N, or 0 when distance is at most Math::Epsilon.
     */
    template <std::floating_point T>
    [[nodiscard]] constexpr T gravitationalForceMagnitude(
        T massA, std::type_identity_t<T> massB, std::type_identity_t<T> distance,
        std::type_identity_t<T> gravitationalConstant = GravitationalConstant<T>) noexcept {
        if (distance <= Math::Epsilon<T>) {
            return T{0};
        }
        return gravitationalConstant * massA * massB / (distance * distance);
    }

    /**
     * @brief Gravitational force that body B exerts on body A.
     *
     * The force points from A towards B. The force on B is its opposite
     * (Newton's third law), so a pair of bodies needs a single call.
     *
     * @return The force in N, or the zero vector when the bodies are at
     *         most Math::Epsilon apart.
     */
    template <std::floating_point T>
    [[nodiscard]] Math::Vector2<T> gravitationalForce(
        const Math::Vector2<T>& positionA, std::type_identity_t<T> massA,
        const Math::Vector2<T>& positionB, std::type_identity_t<T> massB,
        std::type_identity_t<T> gravitationalConstant = GravitationalConstant<T>) noexcept {
        const Math::Vector2<T> towardsB = positionB - positionA;
        const T distanceSquared = towardsB.lengthSquared();
        if (distanceSquared <= Math::Epsilon<T> * Math::Epsilon<T>) {
            return {};
        }
        const T distance = std::sqrt(distanceSquared);
        // towardsB / distance is the unit direction, times G * mA * mB / d².
        return towardsB * (gravitationalConstant * massA * massB / (distanceSquared * distance));
    }

    /**
     * @brief Acceleration that body B gives to body A: G * mB / d².
     *
     * Equals gravitationalForce() / massA, without needing massA: use it to
     * update velocities directly. Points from A towards B.
     *
     * @return The acceleration in m/s², or the zero vector when the bodies
     *         are at most Math::Epsilon apart.
     */
    template <std::floating_point T>
    [[nodiscard]] Math::Vector2<T> gravitationalAcceleration(
        const Math::Vector2<T>& positionA, const Math::Vector2<T>& positionB,
        std::type_identity_t<T> massB,
        std::type_identity_t<T> gravitationalConstant = GravitationalConstant<T>) noexcept {
        return gravitationalForce(positionA, T{1}, positionB, massB, gravitationalConstant);
    }
}
