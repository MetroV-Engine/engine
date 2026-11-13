#pragma once

/**
 * @file
 * @brief Approximate comparisons for the engine's math and physics tests.
 *
 * Independent of Math::approxEqual, so the tests of that function do not
 * check it against itself.
 */

#include <cmath>

#include "Engine/Common/Math/Vector.hpp"

namespace TestSupport {
    template <typename T>
    [[nodiscard]] bool near(T actual, T expected, T tolerance) {
        return std::fabs(actual - expected) <= tolerance;
    }

    template <typename T>
    [[nodiscard]] bool near(
        const Engine::Math::Vector2<T>& actual, const Engine::Math::Vector2<T>& expected, T tolerance) {
        return near(actual.x, expected.x, tolerance) && near(actual.y, expected.y, tolerance);
    }
}
