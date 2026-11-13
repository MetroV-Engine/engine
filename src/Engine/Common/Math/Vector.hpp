#pragma once

/**
 * @file
 * @brief 2D vector type used by the engine's math and physics code.
 *
 * Vector2<T> is a plain value type: two public coordinates, no invariant.
 * Use the aliases rather than the template:
 * - Vector2f: float, the default for positions, velocities and forces.
 * - Vector2d: double, when float precision is not enough.
 * - Vector2i: int, for grid coordinates and sizes in pixels.
 *
 * Operations that need a square root or trigonometry (length, normalized,
 * rotated, angle...) only exist for floating-point vectors: convert an
 * integer vector first, e.g. Vector2f(gridPosition).length().
 *
 * Angles are in radians. A positive angle turns from +X towards +Y, which
 * is counter-clockwise when Y points up and clockwise on screen when Y
 * points down.
 */

#include <cmath>
#include <concepts>
#include <cstdlib>
#include <ostream>
#include <type_traits>

#include "Scalar.hpp"

namespace Engine::Math {
    /**
     * @brief Vector with two coordinates of type T.
     *
     * == and != compare coordinates exactly; floating-point results of
     * computations should be compared with approxEqual() instead.
     */
    template <Scalar T>
    struct Vector2 {
        T x{};
        T y{};

        /** @brief Creates the zero vector. */
        constexpr Vector2() noexcept = default;

        /** @brief Creates the vector (xValue, yValue). */
        constexpr Vector2(T xValue, T yValue) noexcept : x(xValue), y(yValue) {}

        /**
         * @brief Converts a vector holding another coordinate type.
         *
         * Explicit because the conversion can lose precision; converting
         * to an integer type truncates towards zero.
         */
        template <Scalar U>
        constexpr explicit Vector2(const Vector2<U>& other) noexcept
            : x(static_cast<T>(other.x)), y(static_cast<T>(other.y)) {}

        /** @brief (0, 0). */
        [[nodiscard]] static constexpr Vector2 zero() noexcept { return {}; }

        /** @brief (1, 1). */
        [[nodiscard]] static constexpr Vector2 one() noexcept { return {T{1}, T{1}}; }

        /** @brief (1, 0). */
        [[nodiscard]] static constexpr Vector2 unitX() noexcept { return {T{1}, T{0}}; }

        /** @brief (0, 1). */
        [[nodiscard]] static constexpr Vector2 unitY() noexcept { return {T{0}, T{1}}; }

        constexpr Vector2& operator+=(const Vector2& other) noexcept {
            x += other.x;
            y += other.y;
            return *this;
        }

        constexpr Vector2& operator-=(const Vector2& other) noexcept {
            x -= other.x;
            y -= other.y;
            return *this;
        }

        constexpr Vector2& operator*=(T scalar) noexcept {
            x *= scalar;
            y *= scalar;
            return *this;
        }

        /**
         * @pre For integer vectors, scalar is not 0.
         *
         * Floating-point vectors multiply by 1 / scalar: one division
         * instead of two.
         */
        constexpr Vector2& operator/=(T scalar) noexcept {
            if constexpr (std::floating_point<T>) {
                const T inverse = T{1} / scalar;
                x *= inverse;
                y *= inverse;
            } else {
                x /= scalar;
                y /= scalar;
            }
            return *this;
        }

        /** @brief Multiplies each coordinate by the matching one of other. */
        constexpr Vector2& operator*=(const Vector2& other) noexcept {
            x *= other.x;
            y *= other.y;
            return *this;
        }

        /**
         * @brief Divides each coordinate by the matching one of other.
         * @pre For integer vectors, no coordinate of other is 0.
         */
        constexpr Vector2& operator/=(const Vector2& other) noexcept {
            x /= other.x;
            y /= other.y;
            return *this;
        }

        [[nodiscard]] constexpr Vector2 operator-() const noexcept {
            return {static_cast<T>(-x), static_cast<T>(-y)};
        }

        [[nodiscard]] friend constexpr Vector2 operator+(Vector2 lhs, const Vector2& rhs) noexcept {
            return lhs += rhs;
        }

        [[nodiscard]] friend constexpr Vector2 operator-(Vector2 lhs, const Vector2& rhs) noexcept {
            return lhs -= rhs;
        }

        [[nodiscard]] friend constexpr Vector2 operator*(Vector2 vector, T scalar) noexcept {
            return vector *= scalar;
        }

        [[nodiscard]] friend constexpr Vector2 operator*(T scalar, Vector2 vector) noexcept {
            return vector *= scalar;
        }

        /** @pre For integer vectors, scalar is not 0. */
        [[nodiscard]] friend constexpr Vector2 operator/(Vector2 vector, T scalar) noexcept {
            return vector /= scalar;
        }

        /** @brief Component-wise product (not the dot product: see dot()). */
        [[nodiscard]] friend constexpr Vector2 operator*(Vector2 lhs, const Vector2& rhs) noexcept {
            return lhs *= rhs;
        }

        /**
         * @brief Component-wise quotient.
         * @pre For integer vectors, no coordinate of rhs is 0.
         */
        [[nodiscard]] friend constexpr Vector2 operator/(Vector2 lhs, const Vector2& rhs) noexcept {
            return lhs /= rhs;
        }

        [[nodiscard]] constexpr bool operator==(const Vector2& other) const noexcept = default;

        /**
         * @brief Dot product: |this| * |other| * cos(angle between them).
         *
         * Positive when both point the same way, 0 when perpendicular,
         * negative when they point in opposite directions.
         */
        [[nodiscard]] constexpr T dot(const Vector2& other) const noexcept {
            return static_cast<T>(x * other.x + y * other.y);
        }

        /**
         * @brief 2D cross product: the Z coordinate of the 3D cross product.
         *
         * Equals |this| * |other| * sin(angle from this to other): positive
         * when other is reached by turning towards +Y (see the file comment),
         * 0 when both are parallel.
         */
        [[nodiscard]] constexpr T cross(const Vector2& other) const noexcept {
            return static_cast<T>(x * other.y - y * other.x);
        }

        /** @brief Squared length. Cheaper than length() to compare lengths. */
        [[nodiscard]] constexpr T lengthSquared() const noexcept { return dot(*this); }

        /** @brief Euclidean length. */
        [[nodiscard]] T length() const noexcept
            requires std::floating_point<T>
        {
            return std::sqrt(lengthSquared());
        }

        /** @brief Squared distance between the points this and other. */
        [[nodiscard]] constexpr T distanceSquaredTo(const Vector2& other) const noexcept {
            return (other - *this).lengthSquared();
        }

        /** @brief Distance between the points this and other. */
        [[nodiscard]] T distanceTo(const Vector2& other) const noexcept
            requires std::floating_point<T>
        {
            return (other - *this).length();
        }

        /**
         * @brief Vector of length 1 with the same direction.
         * @return The zero vector when the length is at most Epsilon, since
         *         such a vector has no reliable direction.
         */
        [[nodiscard]] Vector2 normalized() const noexcept
            requires std::floating_point<T>
        {
            const T currentLength = length();
            if (currentLength <= Epsilon<T>) {
                return {};
            }
            return *this / currentLength;
        }

        /** @brief This vector turned by +90 degrees: (-y, x). */
        [[nodiscard]] constexpr Vector2 perpendicular() const noexcept {
            return {static_cast<T>(-y), x};
        }

        /** @brief This vector turned by the given angle, in radians. */
        [[nodiscard]] Vector2 rotated(T radians) const noexcept
            requires std::floating_point<T>
        {
            const T cosine = std::cos(radians);
            const T sine = std::sin(radians);
            return {x * cosine - y * sine, x * sine + y * cosine};
        }

        /**
         * @brief Angle from +X to this vector, in radians, in [-pi, pi].
         *
         * The zero vector gives 0.
         */
        [[nodiscard]] T angle() const noexcept
            requires std::floating_point<T>
        {
            return std::atan2(y, x);
        }

        /**
         * @brief Signed angle to turn this vector onto other, in radians,
         *        in [-pi, pi].
         */
        [[nodiscard]] T angleTo(const Vector2& other) const noexcept
            requires std::floating_point<T>
        {
            return std::atan2(cross(other), dot(other));
        }

        /**
         * @brief Component of this vector along the direction of onto.
         * @return The zero vector when onto is shorter than Epsilon.
         */
        [[nodiscard]] Vector2 projectedOnto(const Vector2& onto) const noexcept
            requires std::floating_point<T>
        {
            const T ontoLengthSquared = onto.lengthSquared();
            if (ontoLengthSquared <= Epsilon<T> * Epsilon<T>) {
                return {};
            }
            return onto * (dot(onto) / ontoLengthSquared);
        }

        /**
         * @brief This vector bounced off a surface, like a ball hitting a wall.
         * @param normal Normal of the surface. Must have length 1.
         */
        [[nodiscard]] constexpr Vector2 reflected(const Vector2& normal) const noexcept
            requires std::floating_point<T>
        {
            return *this - normal * (T{2} * dot(normal));
        }

        /**
         * @brief This vector shortened to maxLength if it is longer; unchanged
         *        otherwise.
         * @pre maxLength >= 0.
         */
        [[nodiscard]] Vector2 clampedLength(T maxLength) const noexcept
            requires std::floating_point<T>
        {
            const T currentLengthSquared = lengthSquared();
            if (currentLengthSquared <= maxLength * maxLength) {
                return *this;
            }
            return *this * (maxLength / std::sqrt(currentLengthSquared));
        }

        /**
         * @brief Whether each coordinate differs from the matching one of
         *        other by at most epsilon.
         */
        [[nodiscard]] bool approxEqual(const Vector2& other, T epsilon = Epsilon<T>) const noexcept
            requires std::floating_point<T>
        {
            return std::abs(x - other.x) <= epsilon && std::abs(y - other.y) <= epsilon;
        }

        /** @brief Writes the vector as "(x, y)". */
        friend std::ostream& operator<<(std::ostream& stream, const Vector2& vector) {
            return stream << '(' << vector.x << ", " << vector.y << ')';
        }
    };

    using Vector2f = Vector2<float>;
    using Vector2d = Vector2<double>;
    using Vector2i = Vector2<int>;

    /**
     * @brief Linear interpolation: from when t is 0, to when t is 1.
     *
     * t is not clamped, so values outside [0, 1] extrapolate.
     */
    template <std::floating_point T>
    [[nodiscard]] constexpr Vector2<T> lerp(
        const Vector2<T>& from, const Vector2<T>& to, std::type_identity_t<T> t) noexcept {
        return from + (to - from) * t;
    }

    /**
     * @brief current moved towards target by at most maxDistance, without
     *        overshooting.
     *
     * Accelerates a velocity towards the wanted one by acceleration * dt
     * each step, whatever the direction.
     *
     * @pre maxDistance >= 0.
     */
    template <std::floating_point T>
    [[nodiscard]] Vector2<T> moveTowards(
        const Vector2<T>& current, const Vector2<T>& target,
        std::type_identity_t<T> maxDistance) noexcept {
        const Vector2<T> difference = target - current;
        const T distanceSquared = difference.lengthSquared();
        if (distanceSquared <= maxDistance * maxDistance) {
            return target;
        }
        return current + difference * (maxDistance / std::sqrt(distanceSquared));
    }

    /**
     * @brief Smooth approach of target, independent of the frame rate: see
     *        the scalar damp() in Scalar.hpp.
     */
    template <std::floating_point T>
    [[nodiscard]] Vector2<T> damp(
        const Vector2<T>& current, const Vector2<T>& target, std::type_identity_t<T> rate,
        std::type_identity_t<T> deltaTime) noexcept {
        return lerp(current, target, dampFactor(rate, deltaTime));
    }

    /** @brief Smallest of each pair of coordinates. */
    template <Scalar T>
    [[nodiscard]] constexpr Vector2<T> min(const Vector2<T>& a, const Vector2<T>& b) noexcept {
        return {a.x < b.x ? a.x : b.x, a.y < b.y ? a.y : b.y};
    }

    /** @brief Largest of each pair of coordinates. */
    template <Scalar T>
    [[nodiscard]] constexpr Vector2<T> max(const Vector2<T>& a, const Vector2<T>& b) noexcept {
        return {a.x > b.x ? a.x : b.x, a.y > b.y ? a.y : b.y};
    }

    /** @brief Absolute value of each coordinate. */
    template <Scalar T>
    [[nodiscard]] Vector2<T> abs(const Vector2<T>& vector) noexcept {
        return {static_cast<T>(std::abs(vector.x)), static_cast<T>(std::abs(vector.y))};
    }
}
