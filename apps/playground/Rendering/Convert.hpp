/*
    Created on: 11/17/26
    Filename: Convert.hpp
    Description: Component value types to the renderer's, and back.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#pragma once

#include "Components/Layer.hpp"

#include "Renderer/RendererTypes.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>

inline uint8_t toByte(const float channel)
{
    return static_cast<uint8_t>(std::lround(std::clamp(channel, 0.0f, 1.0f) * 255.0f));
}

inline gfx::Color toColor(const Rgba &color)
{
    return gfx::Color{toByte(color.r), toByte(color.g), toByte(color.b), toByte(color.a)};
}

inline Rgba toRgba(const gfx::Color color)
{
    return Rgba{color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f};
}

inline gfx::Vec2 toVec2(const Vec2f &vector)
{
    return gfx::Vec2{vector.x, vector.y};
}

inline gfx::Layer toLayer(const DrawLayer layer)
{
    switch (layer) {
    case DrawLayer::Background:
        return gfx::Layer::Background;
    case DrawLayer::Foreground:
        return gfx::Layer::Foreground;
    case DrawLayer::Game:
        break;
    }
    return gfx::Layer::Game;
}

inline gfx::Space toSpace(const bool screenSpace)
{
    return screenSpace ? gfx::Space::Screen : gfx::Space::World;
}
