/*
    Created on: 9/22/26
    Filename: RendererTypes.hpp
    Description: Shared POD types for the rendering interface: no backend headers.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#ifndef RENDERER_WRAPPER_RENDERERTYPES_HPP
#define RENDERER_WRAPPER_RENDERERTYPES_HPP

#include <array>
#include <cstdint>
#include <span>
#include <string_view>

namespace gfx {

    class Font;

    struct Vec2 {
        float x = 0.0f;
        float y = 0.0f;
    };

    struct Rect {
        float x = 0.0f;
        float y = 0.0f;
        float w = 0.0f;
        float h = 0.0f;
    };

    struct Color {
        uint8_t r = 0;
        uint8_t g = 0;
        uint8_t b = 0;
        uint8_t a = 255;
    };

    inline constexpr Color White{ 255, 255, 255, 255 };
    inline constexpr Color Black{ 0, 0, 0, 255 };

    // center is the world point shown at the middle of the viewport; zoom > 1 magnifies.
    struct Camera2D {
        Vec2 center;
        float zoom = 1.0f;
    };

    enum class TextureHandle : uint32_t { Invalid = 0 };

    enum class Space : uint8_t { World, Screen };

    // Draw order is Background, Game, Foreground: the lowest number ends up on top.
    enum class Layer : uint8_t { Foreground = 1, Game = 2, Background = 3 };
    inline constexpr int LayerCount = 3;

    // Bucket index in draw order; an invalid cast value falls back to Game.
    constexpr int layerSlot(const Layer layer)
    {
        switch (layer) {
            case Layer::Background: return 0;
            case Layer::Game: return 1;
            case Layer::Foreground: return 2;
        }
        return 1;
    }

    // Field order and sizes are the bgfx vertex layout: Position 2f, TexCoord0 2f, Color0 4u8.
    struct Vertex {
        float x = 0.0f;
        float y = 0.0f;
        float u = 0.0f;
        float v = 0.0f;
        uint8_t r = 255;
        uint8_t g = 255;
        uint8_t b = 255;
        uint8_t a = 255;
    };
    static_assert(sizeof(Vertex) == 20, "Vertex must match the 20-byte bgfx layout");

    struct RectDraw {
        Rect dest;
        Color color;
        Layer layer = Layer::Game;
        Space space = Space::World;
    };

    struct SpriteDraw {
        TextureHandle texture = TextureHandle::Invalid;
        Rect source;                // w or h <= 0 means the whole texture
        Rect dest;
        float rotation = 0.0f;      // radians, clockwise, about the centre of dest
        bool flipX = false;         // mirrors the source horizontally inside dest
        Layer layer = Layer::Game;
        Space space = Space::World;
        Color tint = White;
    };

    struct LineDraw {
        Vec2 from;
        Vec2 to;
        float thickness = 1.0f;
        Color color;
        Layer layer = Layer::Game;
        Space space = Space::World;
    };

    struct TriangleDraw {
        std::array<Vec2, 3> points;
        Color color;
        Layer layer = Layer::Game;
        Space space = Space::World;
    };

    struct QuadDraw {
        std::array<Vec2, 4> corners;    // in order around the shape
        Color color;
        Layer layer = Layer::Game;
        Space space = Space::World;
    };

    struct CircleDraw {
        Vec2 center;
        float radius = 0.0f;
        Color color;
        int segments = 0;               // <= 0 derives the count from the radius
        Layer layer = Layer::Game;
        Space space = Space::World;
    };

    struct PolygonDraw {
        std::span<const Vec2> points;
        Color color;
        Layer layer = Layer::Game;
        Space space = Space::World;
    };

    struct MeshDraw {
        std::span<const Vertex> vertices;
        std::span<const uint16_t> indices;
        TextureHandle texture = TextureHandle::Invalid;
        Layer layer = Layer::Game;
        Space space = Space::World;
    };

    struct TextDraw {
        const Font* font = nullptr;
        std::string_view text;          // UTF-8; only read during the draw call
        Vec2 position;                  // top-left of the first line's box
        Color color = White;
        Layer layer = Layer::Game;
        Space space = Space::World;
    };

}

#endif //RENDERER_WRAPPER_RENDERERTYPES_HPP
