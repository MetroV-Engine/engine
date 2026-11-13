#pragma once

/**
 * @file
 * @brief Rotation of rigid bodies: moments of inertia, torque, the velocity
 *        of a point of a turning body, and integration of the angle.
 *
 * The rotational twin of Dynamics.hpp and Kinematics.hpp:
 * - mass m          <-> moment of inertia I (kg·m²): how hard it is to spin;
 * - force F         <-> torque τ = r x F (N·m), r the lever arm from the
 *                       center of mass to where the force applies;
 * - velocity v      <-> angular velocity ω (rad/s, > 0 counter-clockwise);
 * - F = m a         <-> τ = I α.
 * A point at arm r of a body moves at v + ω x r = v + ω * (-r.y, r.x).
 *
 * Moments of inertia are for uniform plates (2D), about their center of
 * mass; parallelAxis() moves them to another point.
 */

#include <concepts>
#include <cstddef>
#include <type_traits>

#include "../Math/Scalar.hpp"
#include "../Math/Vector.hpp"
#include "../Shapes/Polygon.hpp"

namespace Engine::Physic {
    /** @brief Moment of inertia of a uniform rectangle about its center: m (w² + h²) / 12, with w = 2 halfSize.x. */
    template <std::floating_point T>
    [[nodiscard]] constexpr T boxInertia(T mass, const Math::Vector2<T>& halfSize) noexcept {
        return mass * (halfSize.x * halfSize.x + halfSize.y * halfSize.y) / T{3};
    }

    /** @brief Moment of inertia of a uniform disc about its center: m r² / 2. */
    template <std::floating_point T>
    [[nodiscard]] constexpr T discInertia(T mass, std::type_identity_t<T> radius) noexcept {
        return mass * radius * radius / T{2};
    }

    /**
     * @brief Moment of inertia of a uniform convex polygon about its
     *        centroid (Shapes::Polygon::centroid()).
     *
     * Sum over the triangles from the centroid to each edge (a, b):
     * I = m / (6 Σ a x b) * Σ (a x b)(a.a + a.b + b.b). A rectangle given
     * as a polygon gets boxInertia().
     */
    [[nodiscard]] inline float polygonInertia(float mass, const Shapes::Polygon& polygon) noexcept {
        const Math::Vector2f centroid = polygon.centroid();
        float numerator = 0.f;
        float denominator = 0.f;
        for (std::size_t i = 0; i < polygon.count; ++i) {
            const Math::Vector2f a = polygon.vertex(i) - centroid;
            const Math::Vector2f b = polygon.vertex(i + 1) - centroid;
            const float twice = a.cross(b);
            numerator += twice * (a.dot(a) + a.dot(b) + b.dot(b));
            denominator += twice;
        }
        return mass * numerator / (6.f * denominator);
    }

    /**
     * @brief Moment of inertia about a point at offset from the center of
     *        mass: I + m |offset|² (parallel axis theorem).
     */
    template <std::floating_point T>
    [[nodiscard]] constexpr T parallelAxis(T inertia, std::type_identity_t<T> mass, const Math::Vector2<T>& offset) noexcept {
        return inertia + mass * offset.lengthSquared();
    }

    /**
     * @brief 1 / I, or 0 for a body that does not rotate (inertia at most
     *        Math::Epsilon): torques and impulses then leave it unturned,
     *        as inverseMass() leaves a static body unmoved.
     */
    template <std::floating_point T>
    [[nodiscard]] constexpr T inverseInertia(T inertia) noexcept {
        return inertia <= Math::Epsilon<T> ? T{0} : T{1} / inertia;
    }

    /** @brief Torque of a force applied at arm from the center of mass: r x F, > 0 counter-clockwise. */
    template <std::floating_point T>
    [[nodiscard]] constexpr T torque(const Math::Vector2<T>& arm, const Math::Vector2<T>& force) noexcept {
        return arm.cross(force);
    }

    /** @brief Velocity of the point at arm from the center of mass: v + ω x r. */
    template <std::floating_point T>
    [[nodiscard]] constexpr Math::Vector2<T> velocityAtPoint(
        const Math::Vector2<T>& velocity, std::type_identity_t<T> angularVelocity, const Math::Vector2<T>& arm) noexcept {
        return velocity + arm.perpendicular() * angularVelocity;
    }

    /**
     * @brief One step of rotation, semi-implicit Euler like integrate():
     *        the angular velocity first, then the angle with the new one.
     */
    template <std::floating_point T>
    constexpr void integrateRotation(
        T& angle, T& angularVelocity, std::type_identity_t<T> angularAcceleration, std::type_identity_t<T> deltaTime) noexcept {
        angularVelocity += angularAcceleration * deltaTime;
        angle += angularVelocity * deltaTime;
    }
}
