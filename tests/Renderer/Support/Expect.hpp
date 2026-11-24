// Comparisons that keep the meaning of the GoogleTest checks these tests were written with.
#pragma once

#include <bit>
#include <cmath>
#include <cstdint>

namespace test {

// EXPECT_FLOAT_EQ: at most 4 ULPs apart. The sign-magnitude bits are mapped onto one biased
// unsigned scale, so +0 and -0 are 0 apart. NaN is never equal to anything.
inline bool floatEq(const float a, const float b)
{
    if (std::isnan(a) || std::isnan(b))
        return false;
    const auto biased = [](const float value) {
        constexpr std::uint32_t SignBit = 0x80000000u;
        const auto bits = std::bit_cast<std::uint32_t>(value);
        return (bits & SignBit) != 0 ? ~bits + 1u : bits | SignBit;
    };
    const std::uint32_t x = biased(a);
    const std::uint32_t y = biased(b);
    return (x >= y ? x - y : y - x) <= 4u;
}

// EXPECT_NEAR: the tolerance is inclusive.
template <class A, class B, class T>
bool near(const A a, const B b, const T tolerance)
{
    return std::abs(static_cast<double>(a) - static_cast<double>(b)) <= static_cast<double>(tolerance);
}

} // namespace test
