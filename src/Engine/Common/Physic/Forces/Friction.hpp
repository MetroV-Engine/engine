#pragma once

/**
 * @file
 * @brief Friction between two surfaces in contact (Coulomb's law): F = mu * N.
 *
 * - static: holds a body still as long as the applied force stays within
 *   muStatic * N;
 * - kinetic: muKinetic * N, opposite to the velocity, once the body slides.
 * N is the normal force, the force pressing the body against the surface:
 * mass * g for a body resting on flat ground.
 *
 * Units are SI (kilograms, meters, seconds, newtons). Applied over a time
 * step, kinetic friction can overshoot and reverse a slow velocity instead
 * of stopping the body: velocityAfterKineticFriction() applies it without
 * overshooting, and frictionForceForStep() turns that into the force to add
 * before integrating the step.
 */

#include <cmath>
#include <concepts>
#include <type_traits>

#include "../../Math/Vector.hpp"
#include "Dynamics.hpp"

namespace Engine::Physic {
    /**
     * @brief Strength of Coulomb friction: mu * N.
     * @param frictionCoefficient Static or kinetic coefficient mu, >= 0.
     * @param normalForce Force pressing the body against the surface, in N, >= 0.
     * @return The force in N.
     */
    template <std::floating_point T>
    [[nodiscard]] constexpr T frictionMagnitude(
        T frictionCoefficient, std::type_identity_t<T> normalForce) noexcept {
        return frictionCoefficient * normalForce;
    }

    /**
     * @brief Kinetic friction on a body sliding with the given velocity.
     * @param velocity Velocity of the body relative to the surface, in m/s.
     * @param kineticCoefficient Kinetic coefficient muKinetic, >= 0.
     * @param normalForce Force pressing the body against the surface, in N, >= 0.
     * @return A force of strength muKinetic * N opposite to the velocity,
     *         or the zero vector when the speed is at most Math::Epsilon.
     */
    template <std::floating_point T>
    [[nodiscard]] Math::Vector2<T> kineticFriction(
        const Math::Vector2<T>& velocity, std::type_identity_t<T> kineticCoefficient,
        std::type_identity_t<T> normalForce) noexcept {
        return -velocity.normalized() * frictionMagnitude(kineticCoefficient, normalForce);
    }

    /**
     * @brief Whether static friction keeps a resting body from sliding.
     * @param appliedForce Sum of the other forces along the surface, in N.
     * @param staticCoefficient Static coefficient muStatic, >= 0.
     * @param normalForce Force pressing the body against the surface, in N, >= 0.
     * @return True when |appliedForce| <= muStatic * N.
     */
    template <std::floating_point T>
    [[nodiscard]] constexpr bool staticFrictionHolds(
        const Math::Vector2<T>& appliedForce, std::type_identity_t<T> staticCoefficient,
        std::type_identity_t<T> normalForce) noexcept {
        const T threshold = frictionMagnitude(staticCoefficient, normalForce);
        return appliedForce.lengthSquared() <= threshold * threshold;
    }

    /**
     * @brief Coulomb friction on a body, whether it slides or rests.
     *
     * - Sliding (speed above Math::Epsilon): kinetic friction, opposite to
     *   the velocity.
     * - Resting, applied force within muStatic * N: exactly -appliedForce,
     *   so the body stays still.
     * - Resting, applied force above it: the body starts to slide, so
     *   kinetic friction opposite to the applied force.
     *
     * @param velocity Velocity of the body relative to the surface, in m/s.
     * @param appliedForce Sum of the other forces along the surface, in N.
     * @param staticCoefficient Static coefficient muStatic, >= 0.
     * @param kineticCoefficient Kinetic coefficient muKinetic, >= 0,
     *        usually lower than muStatic.
     * @param normalForce Force pressing the body against the surface, in N, >= 0.
     * @return The friction force, in N.
     */
    template <std::floating_point T>
    [[nodiscard]] Math::Vector2<T> frictionForce(
        const Math::Vector2<T>& velocity, const Math::Vector2<T>& appliedForce,
        std::type_identity_t<T> staticCoefficient, std::type_identity_t<T> kineticCoefficient,
        std::type_identity_t<T> normalForce) noexcept {
        if (velocity.lengthSquared() > Math::Epsilon<T> * Math::Epsilon<T>) {
            return kineticFriction(velocity, kineticCoefficient, normalForce);
        }
        if (staticFrictionHolds(appliedForce, staticCoefficient, normalForce)) {
            return -appliedForce;
        }
        return kineticFriction(appliedForce, kineticCoefficient, normalForce);
    }

    /**
     * @brief Velocity of a sliding body after one step of kinetic friction,
     *        stopping it instead of reversing it.
     *
     * Friction removes muKinetic * N / m * dt from the speed. When that is
     * more than the speed left, the body stops: integrating kineticFriction()
     * as a force would instead make it jitter back and forth.
     *
     * @param velocity Velocity of the body relative to the surface, in m/s.
     * @param kineticCoefficient Kinetic coefficient muKinetic, >= 0.
     * @param normalForce Force pressing the body against the surface, in N, >= 0.
     * @param mass Mass of the body, in kg; a static body is left unchanged.
     * @param deltaTime Length of the step, in s, >= 0.
     * @return The new velocity, in m/s.
     */
    template <std::floating_point T>
    [[nodiscard]] Math::Vector2<T> velocityAfterKineticFriction(
        const Math::Vector2<T>& velocity, std::type_identity_t<T> kineticCoefficient,
        std::type_identity_t<T> normalForce, std::type_identity_t<T> mass,
        std::type_identity_t<T> deltaTime) noexcept {
        const T speedSquared = velocity.lengthSquared();
        if (speedSquared <= Math::Epsilon<T> * Math::Epsilon<T>) {
            return {};
        }
        const T speed = std::sqrt(speedSquared);
        const T speedLoss =
            frictionMagnitude(kineticCoefficient, normalForce) * inverseMass(mass) * deltaTime;
        if (speedLoss >= speed) {
            return {};
        }
        return velocity * ((speed - speedLoss) / speed);
    }

    /**
     * @brief Friction force to add for one step, so that integrating the
     *        step gives exactly Coulomb's friction, without overshooting.
     *
     * Semi-implicit Euler first moves the velocity by the other forces:
     * predicted = velocity + appliedForce / m * dt. Friction then acts on
     * that predicted velocity:
     * - resting (speed at most Math::Epsilon) and held by static friction
     *   (staticFrictionHolds()): the body stays still, velocity 0;
     * - otherwise kinetic friction, which stops the body rather than
     *   reversing it (velocityAfterKineticFriction()).
     * The returned force is m * (velocity wanted - predicted) / dt: added to
     * the other forces and integrated over the same dt, it gives exactly
     * that velocity. Integrating kineticFriction() instead would make a slow
     * body jitter back and forth.
     *
     * @param velocity Velocity of the body along the surface, relative to
     *        the surface, at the start of the step, in m/s.
     * @param appliedForce Sum of the other forces along the surface, in N.
     * @param staticCoefficient Static coefficient muStatic, >= 0.
     * @param kineticCoefficient Kinetic coefficient muKinetic, >= 0,
     *        usually lower than muStatic.
     * @param normalForce Force pressing the body against the surface, in N, >= 0.
     * @param mass Mass of the body, in kg.
     * @param deltaTime Length of the step, in s: the one integration will use.
     * @return The friction force, in N, or the zero vector for a static
     *         body or a step of length 0.
     */
    template <std::floating_point T>
    [[nodiscard]] Math::Vector2<T> frictionForceForStep(
        const Math::Vector2<T>& velocity, const Math::Vector2<T>& appliedForce,
        std::type_identity_t<T> staticCoefficient, std::type_identity_t<T> kineticCoefficient,
        std::type_identity_t<T> normalForce, std::type_identity_t<T> mass,
        std::type_identity_t<T> deltaTime) noexcept {
        const T inverse = inverseMass(mass);
        if (inverse == T{0} || deltaTime <= T{0}) {
            return {};
        }
        const bool resting = velocity.lengthSquared() <= Math::Epsilon<T> * Math::Epsilon<T>;
        const Math::Vector2<T> predicted = velocity + appliedForce * (inverse * deltaTime);
        const Math::Vector2<T> wanted =
            resting && staticFrictionHolds(appliedForce, staticCoefficient, normalForce)
                ? Math::Vector2<T>{}
                : velocityAfterKineticFriction(predicted, kineticCoefficient, normalForce, mass, deltaTime);
        return (wanted - predicted) * (mass / deltaTime);
    }
}
