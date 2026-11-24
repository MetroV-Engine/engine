/*
    Created on: 11/9/26
    Filename: Colors.hpp
    Description: Named colours and colour helpers (hex, HSV, lerp): backend-free.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#ifndef RENDERER_WRAPPER_COLORS_HPP
#define RENDERER_WRAPPER_COLORS_HPP

#include "Renderer/RendererTypes.hpp"

#include <cstdint>
#include <optional>
#include <string_view>

namespace gfx {

    inline constexpr Color Transparent{ 0, 0, 0, 0 }, Red{ 255, 0, 0, 255 }, Green{ 0, 255, 0, 255 }, Blue{ 0, 0, 255, 255 },
        Yellow{ 255, 255, 0, 255 }, Cyan{ 0, 255, 255, 255 }, Magenta{ 255, 0, 255, 255 }, Orange{ 255, 165, 0, 255 }, Gray{ 128, 128, 128, 255 };

    // h in degrees (wraps), s, v, a in 0..1 (clamped).
    struct Hsv {
        float h = 0.0f;
        float s = 0.0f;
        float v = 0.0f;
        float a = 1.0f;
    };

    // 0xRRGGBBAA.
    constexpr Color fromHex(const uint32_t rgba)
    {
        return Color{ static_cast<uint8_t>(rgba >> 24), static_cast<uint8_t>(rgba >> 16),
                      static_cast<uint8_t>(rgba >> 8), static_cast<uint8_t>(rgba) };
    }

    // "#RRGGBB" (alpha 255) or "#RRGGBBAA"; anything else is nullopt.
    std::optional<Color> parseHex(std::string_view text);

    Color fromHsv(const Hsv& hsv);
    Hsv toHsv(Color color);

    // t is clamped to 0..1.
    Color lerp(Color a, Color b, float t);

    constexpr Color withAlpha(const Color color, const uint8_t alpha)
    {
        return Color{ color.r, color.g, color.b, alpha };
    }

    // Per-channel a * b / 255, rounded.
    Color multiply(Color a, Color b);

} // namespace gfx

#endif // RENDERER_WRAPPER_COLORS_HPP
