#pragma once

/**
 * @file
 * @brief Newton's second law and impulses: how forces change a velocity.
 *
 * - A force acts over time: a = F / m, applied every step.
 * - An impulse acts at once: J = F * dt = m * dv. Jumps, dashes and
 *   knockbacks are impulses: they set a velocity change in a single step,
 *   whatever the step length.
 *
 * Units are SI (kilograms, meters, seconds, newtons, N·s for impulses).
 *
 * A mass at most Math::Epsilon is treated as infinite (a static body, such
 * as a wall): forces and impulses do not move it. Physics engines usually
 * mark static bodies this way, with an inverse mass of 0.
 */

#include <concepts>
#include <type_traits>

#include "../../Math/Vector.hpp"

namespace Engine::Physic {
    /**
     * @brief Inverse of a mass: 1 / m, or 0 for a static body.
     * @param mass Mass of the body, in kg.
     * @return 1 / mass in 1/kg, or 0 when mass is at most Math::Epsilon.
     */
    template <std::floating_point T>
    [[nodiscard]] constexpr T inverseMass(T mass) noexcept {
        if (mass <= Math::Epsilon<T>) {
            return T{0};
        }
        return T{1} / mass;
    }

    /**
     * @brief Acceleration a force gives to a body: a = F / m.
     * @param force Sum of the forces on the body, in N.
     * @param mass Mass of the body, in kg.
     * @return The acceleration in m/s², or the zero vector for a static body.
     */
    template <std::floating_point T>
    [[nodiscard]] constexpr Math::Vector2<T> accelerationFromForce(
        const Math::Vector2<T>& force, std::type_identity_t<T> mass) noexcept {
        return force * inverseMass(mass);
    }

    /**
     * @brief Force needed to give a body an acceleration: F = m * a.
     * @param mass Mass of the body, in kg.
     * @param acceleration Wanted acceleration, in m/s².
     * @return The force in N.
     */
    template <std::floating_point T>
    [[nodiscard]] constexpr Math::Vector2<T> forceForAcceleration(
        std::type_identity_t<T> mass, const Math::Vector2<T>& acceleration) noexcept {
        return acceleration * mass;
    }

    /**
     * @brief Impulse of a constant force applied for a duration: J = F * dt.
     * @param force The force, in N.
     * @param duration How long it is applied, in s.
     * @return The impulse in N·s.
     */
    template <std::floating_point T>
    [[nodiscard]] constexpr Math::Vector2<T> impulseFromForce(
        const Math::Vector2<T>& force, std::type_identity_t<T> duration) noexcept {
        return force * duration;
    }

    /**
     * @brief Velocity change an impulse gives to a body: dv = J / m.
     * @param impulse The impulse, in N·s.
     * @param mass Mass of the body, in kg.
     * @return The velocity change in m/s, or the zero vector for a static body.
     */
    template <std::floating_point T>
    [[nodiscard]] constexpr Math::Vector2<T> velocityChangeFromImpulse(
        const Math::Vector2<T>& impulse, std::type_identity_t<T> mass) noexcept {
        return impulse * inverseMass(mass);
    }

    /**
     * @brief Impulse needed to change the velocity of a body: J = m * dv.
     *
     * Use it to express a jump, dash or knockback as a velocity change,
     * which stays the same whatever the mass, then apply the impulse.
     *
     * @param mass Mass of the body, in kg.
     * @param velocityChange Wanted velocity change, in m/s.
     * @return The impulse in N·s.
     */
    template <std::floating_point T>
    [[nodiscard]] constexpr Math::Vector2<T> impulseForVelocityChange(
        std::type_identity_t<T> mass, const Math::Vector2<T>& velocityChange) noexcept {
        return velocityChange * mass;
    }
}
