#pragma once

/**
 * @file
 * @brief Drag: what slows a body moving through air or water.
 *
 * - Linear drag, F = -c v: slow motion in a thick fluid (water, mud), or
 *   simply a way to make motion die out.
 * - Quadratic drag, F = -c |v| v: fast motion through air, a terminal
 *   velocity when falling.
 *
 * Both oppose the velocity relative to the fluid: the body's velocity minus
 * the wind's or the current's.
 *
 * An explicit step overshoots a strong drag: the body reverses instead of
 * stopping once c dt / m passes 1 (2 for linear drag). The *ForStep versions
 * give the force to integrate over one step instead: exactly the continuous
 * solution for linear drag (v e^(-c dt / m)), and at most what stops the
 * body for quadratic drag. Same idea as frictionForceForStep.
 *
 * Units are SI (kilograms, meters, seconds, newtons).
 */

#include <algorithm>
#include <cmath>
#include <concepts>
#include <type_traits>

#include "../../Math/Vector.hpp"

namespace Engine::Physic {
    /**
     * @brief Linear drag, F = -c v.
     * @param relativeVelocity Velocity of the body relative to the fluid, in m/s.
     * @param coefficient c, in N·s/m, >= 0.
     * @return The force in N.
     */
    template <std::floating_point T>
    [[nodiscard]] constexpr Math::Vector2<T> linearDrag(
        const Math::Vector2<T>& relativeVelocity, std::type_identity_t<T> coefficient) noexcept {
        return relativeVelocity * -coefficient;
    }

    /**
     * @brief Linear drag for one step, exact whatever the coefficient.
     *
     * Under drag alone, dv/dt = -(c / m) v gives v(dt) = v e^(-c dt / m).
     * The returned force, integrated over dt, gives exactly that velocity
     * change: m (v e^(-c dt / m) - v) / dt. It never reverses the body.
     *
     * @param relativeVelocity Velocity of the body relative to the fluid, in m/s.
     * @param coefficient c, in N·s/m, >= 0.
     * @param mass Mass of the body, in kg; a static body gets nothing.
     * @param deltaTime Length of the step, in s.
     * @return The force in N.
     */
    template <std::floating_point T>
    [[nodiscard]] Math::Vector2<T> linearDragForStep(
        const Math::Vector2<T>& relativeVelocity, std::type_identity_t<T> coefficient,
        std::type_identity_t<T> mass, std::type_identity_t<T> deltaTime) noexcept {
        if (mass <= Math::Epsilon<T> || deltaTime <= T{0}) {
            return {};
        }
        return relativeVelocity * (mass * (std::exp(-coefficient * deltaTime / mass) - T{1}) / deltaTime);
    }

    /**
     * @brief Quadratic drag, F = -c |v| v.
     * @param relativeVelocity Velocity of the body relative to the fluid, in m/s.
     * @param coefficient c, in N·s²/m², >= 0. A body of mass m falling under
     *        gravity g reaches the terminal velocity sqrt(m g / c).
     * @return The force in N.
     */
    template <std::floating_point T>
    [[nodiscard]] Math::Vector2<T> quadraticDrag(
        const Math::Vector2<T>& relativeVelocity, std::type_identity_t<T> coefficient) noexcept {
        return relativeVelocity * (-coefficient * relativeVelocity.length());
    }

    /**
     * @brief Quadratic drag for one step: at most the force that stops the
     *        body within the step, so it never reverses it.
     * @param relativeVelocity Velocity of the body relative to the fluid, in m/s.
     * @param coefficient c, in N·s²/m², >= 0.
     * @param mass Mass of the body, in kg; a static body gets nothing.
     * @param deltaTime Length of the step, in s.
     * @return The force in N.
     */
    template <std::floating_point T>
    [[nodiscard]] Math::Vector2<T> quadraticDragForStep(
        const Math::Vector2<T>& relativeVelocity, std::type_identity_t<T> coefficient,
        std::type_identity_t<T> mass, std::type_identity_t<T> deltaTime) noexcept {
        if (mass <= Math::Epsilon<T> || deltaTime <= T{0}) {
            return {};
        }
        const T speed = relativeVelocity.length();
        const T magnitude = coefficient * speed * speed;
        const T stopping = mass * speed / deltaTime;  // removes the whole relative speed in one step
        return relativeVelocity * (-std::min(magnitude, stopping) / (speed > T{0} ? speed : T{1}));
    }
}