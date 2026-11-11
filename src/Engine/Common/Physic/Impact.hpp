#pragma once

/**
 * @file
 * @brief Impact of two bodies: the impulse that makes them bounce apart
 *        (restitution), and the push that stops them overlapping.
 *
 * What a collision response applies to two bodies A and B found touching,
 * with the unit normal from A towards B (Collision::Contact::normal):
 *
 * - Velocity: bounceImpulse() stops them moving into each other and sends
 *   them apart at e times the speed they met at. e, the coefficient of
 *   restitution, is 0 for a landing (no bounce) and 1 for a bounce that
 *   loses no energy. B gets the impulse, A the opposite:
 *   vB += J / mB, vA -= J / mA (velocityChangeFromImpulse in Dynamics.hpp).
 * - Position: separation() moves them apart by the depth of the overlap,
 *   so that they are left touching.
 *
 * Both act along the normal only: what slows a body along the surface is
 * friction. A mass at most Math::Epsilon is static (Dynamics.hpp): the
 * other body takes the whole bounce and the whole push.
 *
 * Units are SI (kilograms, meters, seconds, N·s for impulses).
 */

#include <algorithm>
#include <concepts>
#include <type_traits>

#include "../Math/Vector.hpp"
#include "Forces/Dynamics.hpp"

namespace Engine::Physic {
    /**
     * @brief Coefficient of restitution of two materials in contact: the
     *        bouncier one, max(eA, eB).
     *
     * A rubber ball bounces on stone as it does on rubber. Other engines
     * average the two or multiply them; max keeps a bouncy object bouncy
     * on every surface.
     */
    template <std::floating_point T>
    [[nodiscard]] constexpr T combinedRestitution(T restitutionA, std::type_identity_t<T> restitutionB) noexcept {
        return std::max(restitutionA, restitutionB);
    }

    /**
     * @brief Speed at which two bodies come together along the normal:
     *        (vA - vB) . n.
     * @param velocityA Velocity of A, in m/s.
     * @param velocityB Velocity of B, in m/s.
     * @param normal Unit normal from A towards B.
     * @return In m/s: > 0 when they approach, 0 when they slide along each
     *         other, < 0 when they move apart.
     */
    template <std::floating_point T>
    [[nodiscard]] constexpr T closingSpeed(
        const Math::Vector2<T>& velocityA, const Math::Vector2<T>& velocityB, const Math::Vector2<T>& normal) noexcept {
        return (velocityA - velocityB).dot(normal);
    }

    /**
     * @brief Impulse that makes two bodies bounce apart: B gets it, A gets
     *        the opposite.
     *
     * With s the closing speed, j = (1 + e) * s / (1 / mA + 1 / mB) along
     * the normal. After it:
     * - they move apart at e * s: the new closing speed is s - j (1 / mA + 1 / mB) = -e * s;
     * - their momentum is unchanged: mA dvA + mB dvB = -j + j = 0;
     * - their kinetic energy drops by (1 - e^2) / 2 * s^2 / (1 / mA + 1 / mB):
     *   nothing at e = 1, never a gain for e in [0, 1];
     * - their velocities along the surface are unchanged.
     *
     * Bodies that are not approaching (s <= 0) get nothing: touching bodies
     * moving apart, or sliding along each other, are left alone.
     *
     * @param velocityA Velocity of A, in m/s.
     * @param velocityB Velocity of B, in m/s.
     * @param normal Unit normal from A towards B (Collision::Contact::normal).
     * @param massA Mass of A, in kg; at most Math::Epsilon: static.
     * @param massB Mass of B, in kg; at most Math::Epsilon: static.
     * @param restitution Coefficient of restitution e, in [0, 1].
     * @param restingSpeed Closing speed under which the bodies do not bounce
     *        (e is taken as 0), in m/s, >= 0. A body resting on the ground is
     *        pulled into it by g * dt every step (0.16 m/s at 60 Hz under
     *        9.81 m/s²): if that bounced, it would hop on the spot. Set it
     *        above g * dt, for example 1 m/s.
     * @return The impulse on B, in N·s; A gets the opposite. The zero vector
     *         when the bodies are not approaching, or both static.
     */
    template <std::floating_point T>
    [[nodiscard]] constexpr Math::Vector2<T> bounceImpulse(
        const Math::Vector2<T>& velocityA, const Math::Vector2<T>& velocityB, const Math::Vector2<T>& normal,
        std::type_identity_t<T> massA, std::type_identity_t<T> massB, std::type_identity_t<T> restitution,
        std::type_identity_t<T> restingSpeed) noexcept {
        const T closing = closingSpeed(velocityA, velocityB, normal);
        const T inverseMasses = inverseMass(massA) + inverseMass(massB);
        if (closing <= T{0} || inverseMasses == T{0}) {
            return {};
        }
        const T e = closing < restingSpeed ? T{0} : restitution;
        return normal * ((T{1} + e) * closing / inverseMasses);
    }

    /** @brief How far each of two overlapping bodies moves to stop overlapping, in m. */
    template <std::floating_point T>
    struct Separation {
        /** @brief Move of A. */
        Math::Vector2<T> first;

        /** @brief Move of B. */
        Math::Vector2<T> second;
    };

    /**
     * @brief Moves that separate two overlapping bodies: the depth, shared
     *        out by inverse mass.
     *
     * The lighter body moves more: one twice as heavy moves half as far, so
     * their center of mass stays where it is (mA * first + mB * second = 0).
     * A static body does not move: the other one moves the whole depth,
     * exactly. Afterwards the bodies touch: second - first = normal * depth.
     *
     * @param normal Unit normal from A towards B (Collision::Contact::normal).
     * @param depth Penetration along the normal, in m, >= 0 (Collision::Contact::depth).
     * @param massA Mass of A, in kg; at most Math::Epsilon: static.
     * @param massB Mass of B, in kg; at most Math::Epsilon: static.
     * @return The moves of A and B; both zero when both bodies are static.
     */
    template <std::floating_point T>
    [[nodiscard]] constexpr Separation<T> separation(
        const Math::Vector2<T>& normal, std::type_identity_t<T> depth,
        std::type_identity_t<T> massA, std::type_identity_t<T> massB) noexcept {
        const T inverseA = inverseMass(massA);
        const T inverseB = inverseMass(massB);
        const T inverseMasses = inverseA + inverseB;
        if (inverseMasses == T{0}) {
            return {};
        }
        // Shares as fractions: against a static body, inverseB / inverseB is exactly 1.
        return {normal * (-depth * (inverseA / inverseMasses)), normal * (depth * (inverseB / inverseMasses))};
    }
}
