#pragma once

/**
 * @file
 * @brief Scalar math shared by the whole engine: the Scalar concept, the
 *        Epsilon tolerance, angles, interpolation and smoothing.
 *
 * Everything is header-only, allocation-free and inlined. Functions are
 * constexpr unless they need <cmath> (sqrt, exp, trigonometry), which is
 * not constexpr before C++26.
 *
 * Angles are in radians.
 */

#include <cmath>
#include <concepts>
#include <numbers>
#include <type_traits>

namespace Engine::Math {
    /**
     * @brief Number types the math code works with.
     *
     * Unsigned types are excluded: negation and subtraction would wrap
     * around instead of producing a negative value.
     */
    template <typename T>
    concept Scalar = std::signed_integral<T> || std::floating_point<T>;

    /**
     * @brief Absolute tolerance of the whole engine: approximate comparisons,
     *        and the lengths below which a vector has no reliable direction.
     */
    template <std::floating_point T>
    inline constexpr T Epsilon = static_cast<T>(1e-5);

    /** @brief Half a turn, in radians. */
    template <std::floating_point T>
    inline constexpr T Pi = std::numbers::pi_v<T>;

    /** @brief A full turn, in radians. */
    template <std::floating_point T>
    inline constexpr T Tau = T{2} * std::numbers::pi_v<T>;

    /** @brief Converts degrees to radians. */
    template <std::floating_point T>
    [[nodiscard]] constexpr T toRadians(T degrees) noexcept {
        return degrees * (Pi<T> / T{180});
    }

    /** @brief Converts radians to degrees. */
    template <std::floating_point T>
    [[nodiscard]] constexpr T toDegrees(T radians) noexcept {
        return radians * (T{180} / Pi<T>);
    }

    /** @brief The same angle brought into [-pi, pi]. */
    template <std::floating_point T>
    [[nodiscard]] T wrapAngle(T radians) noexcept {
        return std::remainder(radians, Tau<T>);
    }

    /** @brief -1, 0 or 1, the sign of value. */
    template <Scalar T>
    [[nodiscard]] constexpr T sign(T value) noexcept {
        return static_cast<T>((T{0} < value) - (value < T{0}));
    }

    /** @brief Whether a and b differ by at most epsilon. */
    template <std::floating_point T>
    [[nodiscard]] constexpr bool approxEqual(
        T a, std::type_identity_t<T> b, std::type_identity_t<T> epsilon = Epsilon<T>) noexcept {
        const T difference = a - b;
        return difference <= epsilon && -difference <= epsilon;
    }

    /**
     * @brief Linear interpolation: from when t is 0, to when t is 1.
     *
     * t is not clamped, so values outside [0, 1] extrapolate. Faster than
     * std::lerp, which pays for exactness guarantees games do not need.
     */
    template <std::floating_point T>
    [[nodiscard]] constexpr T lerp(
        T from, std::type_identity_t<T> to, std::type_identity_t<T> t) noexcept {
        return from + (to - from) * t;
    }

    /**
     * @brief Inverse of lerp: where value lies between from (0) and to (1).
     * @return The position, or 0 when from and to are at most Epsilon apart.
     */
    template <std::floating_point T>
    [[nodiscard]] constexpr T inverseLerp(
        T from, std::type_identity_t<T> to, std::type_identity_t<T> value) noexcept {
        const T range = to - from;
        if (approxEqual(range, T{0})) {
            return T{0};
        }
        return (value - from) / range;
    }

    /**
     * @brief Maps value from [inFrom, inTo] to [outFrom, outTo], keeping its
     *        relative position. Not clamped.
     */
    template <std::floating_point T>
    [[nodiscard]] constexpr T remap(
        T value, std::type_identity_t<T> inFrom, std::type_identity_t<T> inTo,
        std::type_identity_t<T> outFrom, std::type_identity_t<T> outTo) noexcept {
        return lerp(outFrom, outTo, inverseLerp(inFrom, inTo, value));
    }

    /**
     * @brief current moved towards target by at most maxDelta, without
     *        overshooting.
     *
     * The base of platformer movement: accelerate a speed towards the
     * wanted speed by acceleration * dt each step.
     *
     * @pre maxDelta >= 0.
     */
    template <std::floating_point T>
    [[nodiscard]] constexpr T moveTowards(
        T current, std::type_identity_t<T> target, std::type_identity_t<T> maxDelta) noexcept {
        const T difference = target - current;
        if (difference <= maxDelta && -difference <= maxDelta) {
            return target;
        }
        return current + sign(difference) * maxDelta;
    }

    /**
     * @brief Share of the remaining distance that damp() covers in one step:
     *        1 - e^(-rate * dt).
     *
     * Compute it once per step when damping many values with the same rate.
     */
    template <std::floating_point T>
    [[nodiscard]] T dampFactor(T rate, std::type_identity_t<T> deltaTime) noexcept {
        return T{1} - std::exp(-rate * deltaTime);
    }

    /**
     * @brief Smooth approach of target, independent of the frame rate.
     *
     * Covers the same share of the distance per second whatever the step
     * length, unlike lerp(current, target, constant). For cameras, UI and
     * any value that should ease towards a target.
     *
     * @param rate How fast target is approached, in 1/s: about 99% of the
     *        distance is covered after 4.6 / rate seconds.
     */
    template <std::floating_point T>
    [[nodiscard]] T damp(
        T current, std::type_identity_t<T> target, std::type_identity_t<T> rate,
        std::type_identity_t<T> deltaTime) noexcept {
        return lerp(current, target, dampFactor(rate, deltaTime));
    }
}
