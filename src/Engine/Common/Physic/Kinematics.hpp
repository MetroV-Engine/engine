#pragma once

/**
 * @file
 * @brief Motion over time: integration of a step, and the closed-form
 *        trajectory under a constant acceleration (jumps, projectiles).
 *
 * Forces give an acceleration (Forces/Dynamics.hpp); this file turns it
 * into a velocity and a position.
 *
 * Integration uses semi-implicit Euler: velocity first, then position with
 * the new velocity. It costs the same as explicit Euler but stays stable
 * for springs and orbits, where explicit Euler gains energy and diverges.
 * Call it with a fixed time step (e.g. 1/60 s, with an accumulator in the
 * game loop): with a variable step, jump heights change with the frame
 * rate.
 *
 * Units are SI (meters, seconds, m/s, m/s²).
 */

#include <cmath>
#include <concepts>
#include <type_traits>

#include "../Math/Vector.hpp"

namespace Engine::Physic {
    /**
     * @brief Advances a body by one time step (semi-implicit Euler).
     * @param position Position of the body, in m, updated in place.
     * @param velocity Velocity of the body, in m/s, updated in place.
     * @param acceleration Acceleration during the step, in m/s².
     * @param deltaTime Length of the step, in s.
     */
    template <std::floating_point T>
    constexpr void integrate(
        Math::Vector2<T>& position, Math::Vector2<T>& velocity,
        const Math::Vector2<T>& acceleration, std::type_identity_t<T> deltaTime) noexcept {
        velocity += acceleration * deltaTime;
        position += velocity * deltaTime;
    }

    /**
     * @brief Velocity after a time under a constant acceleration: v0 + a * t.
     * @param initialVelocity Velocity at time 0, in m/s.
     * @param acceleration Constant acceleration, in m/s².
     * @param time Elapsed time, in s.
     */
    template <std::floating_point T>
    [[nodiscard]] constexpr Math::Vector2<T> velocityAtTime(
        const Math::Vector2<T>& initialVelocity, const Math::Vector2<T>& acceleration,
        std::type_identity_t<T> time) noexcept {
        return initialVelocity + acceleration * time;
    }

    /**
     * @brief Position after a time under a constant acceleration:
     *        p0 + v0 * t + a * t² / 2.
     *
     * Exact, whatever the time: use it to predict where a projectile lands
     * or to draw a throw's arc.
     *
     * @param initialPosition Position at time 0, in m.
     * @param initialVelocity Velocity at time 0, in m/s.
     * @param acceleration Constant acceleration, in m/s².
     * @param time Elapsed time, in s.
     */
    template <std::floating_point T>
    [[nodiscard]] constexpr Math::Vector2<T> positionAtTime(
        const Math::Vector2<T>& initialPosition, const Math::Vector2<T>& initialVelocity,
        const Math::Vector2<T>& acceleration, std::type_identity_t<T> time) noexcept {
        return initialPosition + initialVelocity * time + acceleration * (T{0.5} * time * time);
    }

    /**
     * @brief Initial upward speed needed to rise by a height under uniform
     *        gravity: v = sqrt(2 * g * h).
     *
     * Lets a jump be designed by its height: the jump impulse is then
     * impulseForVelocityChange(mass, up * speedToReachHeight(h, g)).
     *
     * @param height Height to reach, in m, >= 0.
     * @param gravityMagnitude Strength of gravity |g|, in m/s², >= 0.
     * @return The speed in m/s.
     */
    template <std::floating_point T>
    [[nodiscard]] T speedToReachHeight(T height, std::type_identity_t<T> gravityMagnitude) noexcept {
        return std::sqrt(T{2} * gravityMagnitude * height);
    }

    /**
     * @brief Initial upward speed to rise by a height when integrate() moves
     *        the body in steps of deltaTime: the speedToReachHeight() of
     *        the simulation rather than of the continuous motion.
     *
     * Semi-implicit Euler lowers the velocity before each move, so a body
     * launched at sqrt(2 g h) peaks about v * dt / 2 short of h: 12 cm
     * (5%) for a 2.5 m jump at 60 Hz. Summing the steps' moves while the
     * velocity is positive gives h = v² / (2 g) - v * dt / 2, up to at most
     * g * dt² / 8 (0.3 mm at 60 Hz, and never below h: the body always
     * clears a ledge of that height); solved for v:
     * v = g dt / 2 + sqrt((g dt / 2)² + 2 g h). With deltaTime 0, this is
     * speedToReachHeight().
     *
     * @param height Height to reach, in m, >= 0.
     * @param gravityMagnitude Strength of gravity |g|, in m/s², >= 0.
     * @param deltaTime Length of the integration steps, in s, >= 0.
     * @return The speed in m/s.
     */
    template <std::floating_point T>
    [[nodiscard]] T speedToReachHeightForStep(
        T height, std::type_identity_t<T> gravityMagnitude, std::type_identity_t<T> deltaTime) noexcept {
        const T halfStepLoss = gravityMagnitude * deltaTime / T{2};
        return halfStepLoss + std::sqrt(halfStepLoss * halfStepLoss + T{2} * gravityMagnitude * height);
    }

    /**
     * @brief Height reached by a body launched upwards: h = v² / (2 * g).
     * @param launchSpeed Initial upward speed, in m/s.
     * @param gravityMagnitude Strength of gravity |g|, in m/s².
     * @pre gravityMagnitude > 0.
     * @return The height above the launch point, in m.
     */
    template <std::floating_point T>
    [[nodiscard]] constexpr T apexHeight(T launchSpeed, std::type_identity_t<T> gravityMagnitude) noexcept {
        return launchSpeed * launchSpeed / (T{2} * gravityMagnitude);
    }

    /**
     * @brief Time for a body launched upwards to reach its highest point:
     *        t = v / g.
     *
     * The whole jump, back to the launch height, lasts twice as long.
     *
     * @param launchSpeed Initial upward speed, in m/s.
     * @param gravityMagnitude Strength of gravity |g|, in m/s².
     * @pre gravityMagnitude > 0.
     * @return The time in s.
     */
    template <std::floating_point T>
    [[nodiscard]] constexpr T timeToApex(T launchSpeed, std::type_identity_t<T> gravityMagnitude) noexcept {
        return launchSpeed / gravityMagnitude;
    }
}
