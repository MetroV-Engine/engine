/*
    Created on: 11/9/26
    Filename: Colors.cpp
    Description: Colour helper implementations.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "Renderer/Colors/Colors.hpp"

#include <algorithm>
#include <cmath>

namespace gfx {

    namespace {

        int hexDigit(const char c)
        {
            if (c >= '0' && c <= '9') return c - '0';
            if (c >= 'a' && c <= 'f') return c - 'a' + 10;
            if (c >= 'A' && c <= 'F') return c - 'A' + 10;
            return -1;
        }

        float clamp01(const float value)
        {
            return std::clamp(value, 0.0f, 1.0f);
        }

        uint8_t toByte(const float unit)
        {
            return static_cast<uint8_t>(std::lround(clamp01(unit) * 255.0f));
        }

        uint8_t lerpChannel(const uint8_t a, const uint8_t b, const float t)
        {
            const float value = static_cast<float>(a) + (static_cast<float>(b) - static_cast<float>(a)) * t;
            return static_cast<uint8_t>(std::lround(std::clamp(value, 0.0f, 255.0f)));
        }

        uint8_t multiplyChannel(const uint8_t a, const uint8_t b)
        {
            return static_cast<uint8_t>((static_cast<unsigned>(a) * b + 127u) / 255u);
        }

    } // namespace

    std::optional<Color> parseHex(const std::string_view text)
    {
        if ((text.size() != 7 && text.size() != 9) || text[0] != '#') return std::nullopt;

        uint32_t value = 0;
        for (size_t i = 1; i < text.size(); ++i) {
            const int digit = hexDigit(text[i]);
            if (digit < 0) return std::nullopt;
            value = (value << 4) | static_cast<uint32_t>(digit);
        }
        if (text.size() == 7) value = (value << 8) | 0xFFu;
        return fromHex(value);
    }

    Color fromHsv(const Hsv& hsv)
    {
        const float s = clamp01(hsv.s);
        const float v = clamp01(hsv.v);

        float hue = std::fmod(hsv.h, 360.0f);
        if (hue < 0.0f) hue += 360.0f;

        const float sector = hue / 60.0f;
        const int index = std::min(static_cast<int>(sector), 5);
        const float fraction = sector - static_cast<float>(index);

        const float p = v * (1.0f - s);
        const float q = v * (1.0f - s * fraction);
        const float t = v * (1.0f - s * (1.0f - fraction));

        float r = v, g = t, b = p;
        switch (index) {
            case 1: r = q; g = v; b = p; break;
            case 2: r = p; g = v; b = t; break;
            case 3: r = p; g = q; b = v; break;
            case 4: r = t; g = p; b = v; break;
            case 5: r = v; g = p; b = q; break;
            default: break;
        }
        return Color{ toByte(r), toByte(g), toByte(b), toByte(hsv.a) };
    }

    Hsv toHsv(const Color color)
    {
        const float r = static_cast<float>(color.r) / 255.0f;
        const float g = static_cast<float>(color.g) / 255.0f;
        const float b = static_cast<float>(color.b) / 255.0f;

        const float maxValue = std::max({ r, g, b });
        const float delta = maxValue - std::min({ r, g, b });

        float hue = 0.0f;
        if (delta > 0.0f) {
            if (maxValue == r) hue = 60.0f * std::fmod((g - b) / delta, 6.0f);
            else if (maxValue == g) hue = 60.0f * ((b - r) / delta + 2.0f);
            else hue = 60.0f * ((r - g) / delta + 4.0f);
            if (hue < 0.0f) hue += 360.0f;
        }

        const float saturation = maxValue > 0.0f ? delta / maxValue : 0.0f;
        return Hsv{ hue, saturation, maxValue, static_cast<float>(color.a) / 255.0f };
    }

    Color lerp(const Color a, const Color b, const float t)
    {
        const float u = clamp01(t);
        return Color{ lerpChannel(a.r, b.r, u), lerpChannel(a.g, b.g, u),
                      lerpChannel(a.b, b.b, u), lerpChannel(a.a, b.a, u) };
    }

    Color multiply(const Color a, const Color b)
    {
        return Color{ multiplyChannel(a.r, b.r), multiplyChannel(a.g, b.g),
                      multiplyChannel(a.b, b.b), multiplyChannel(a.a, b.a) };
    }

} // namespace gfx
