#include <boost/ut.hpp>

#include "Support/Expect.hpp"
#include "Renderer/Colors/Colors.hpp"

#include <cmath>
#include <cstdint>
#include <optional>

using namespace boost::ut;

namespace {

    void expectColor(const gfx::Color color, const int r, const int g, const int b, const int a)
    {
        expect(color.r == r);
        expect(color.g == g);
        expect(color.b == b);
        expect(color.a == a);
    }

    void expectNear(const gfx::Color actual, const gfx::Color expected)
    {
        expect(test::near(actual.r, expected.r, 1));
        expect(test::near(actual.g, expected.g, 1));
        expect(test::near(actual.b, expected.b, 1));
        expect(test::near(actual.a, expected.a, 1));
    }

} // namespace

void run_colors_tests()
{
    "colors: from hex splits channels"_test = [] {
        expectColor(gfx::fromHex(0x11223344), 0x11, 0x22, 0x33, 0x44);
    };

    "colors: parse hex reads both lengths"_test = [] {
        const auto rgb = gfx::parseHex("#112233");
        expect(rgb.has_value()) << fatal;
        expectColor(*rgb, 0x11, 0x22, 0x33, 255);

        const auto rgba = gfx::parseHex("#11223344");
        expect(rgba.has_value()) << fatal;
        expectColor(*rgba, 0x11, 0x22, 0x33, 0x44);

        const auto mixed = gfx::parseHex("#aaBBcc");
        expect(mixed.has_value()) << fatal;
        expectColor(*mixed, 0xAA, 0xBB, 0xCC, 255);
    };

    "colors: parse hex rejects bad input"_test = [] {
        expect((gfx::parseHex("")) == std::nullopt);
        expect((gfx::parseHex("112233")) == std::nullopt);
        expect((gfx::parseHex("#12345")) == std::nullopt);
        expect((gfx::parseHex("#12345G")) == std::nullopt);
        expect((gfx::parseHex("#1122334455")) == std::nullopt);
    };

    "colors: hsv primaries"_test = [] {
        expectColor(gfx::fromHsv({ 0.0f, 1.0f, 1.0f }), 255, 0, 0, 255);
        expectColor(gfx::fromHsv({ 120.0f, 1.0f, 1.0f }), 0, 255, 0, 255);
        expectColor(gfx::fromHsv({ 240.0f, 1.0f, 1.0f }), 0, 0, 255, 255);
        expectColor(gfx::fromHsv({ 0.0f, 0.0f, 1.0f }), 255, 255, 255, 255);
    };

    "colors: hsv wraps hue and clamps"_test = [] {
        expectColor(gfx::fromHsv({ 360.0f, 1.0f, 1.0f }), 255, 0, 0, 255);
        expectColor(gfx::fromHsv({ -360.0f, 1.0f, 1.0f }), 255, 0, 0, 255);
        expectColor(gfx::fromHsv({ 0.0f, 2.0f, 2.0f, 2.0f }), 255, 0, 0, 255);
    };

    "colors: to hsv round trips"_test = [] {
        for (const gfx::Color c : { gfx::Red, gfx::Orange, gfx::Gray, gfx::Cyan }) {
            expectNear(gfx::fromHsv(gfx::toHsv(c)), c);
        }
    };

    "colors: to hsv of black has zero saturation"_test = [] {
        const gfx::Hsv hsv = gfx::toHsv(gfx::Black);
        expect(hsv.s == 0.0f);
        expect(!std::isnan(hsv.h));
        expect(!std::isnan(hsv.s));
        expect(!std::isnan(hsv.v));
    };

    "colors: lerp ends and clamps"_test = [] {
        const gfx::Color a{ 0, 10, 20, 30 };
        const gfx::Color b{ 255, 200, 100, 255 };
        expectColor(gfx::lerp(a, b, 0.0f), 0, 10, 20, 30);
        expectColor(gfx::lerp(a, b, 1.0f), 255, 200, 100, 255);
        expectColor(gfx::lerp(a, b, -1.0f), 0, 10, 20, 30);
        expectColor(gfx::lerp(a, b, 2.0f), 255, 200, 100, 255);

        const int mid = gfx::lerp(a, b, 0.5f).r;
        expect(mid == 127 || mid == 128);
    };

    "colors: with alpha keeps rgb"_test = [] {
        expectColor(gfx::withAlpha(gfx::Orange, 10), 255, 165, 0, 10);
    };

    "colors: multiply white is identity and black is zero"_test = [] {
        const gfx::Color c{ 12, 34, 56, 78 };
        expectColor(gfx::multiply(c, gfx::White), 12, 34, 56, 78);
        expectColor(gfx::multiply(c, gfx::Black), 0, 0, 0, 78);
        expectColor(gfx::multiply({ 128, 128, 128, 255 }, { 128, 128, 128, 255 }), 64, 64, 64, 255);
    };
}
