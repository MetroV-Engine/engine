/*
    Created on: 10/4/26
    Filename: QuadBuilder.cpp
    Description: Turns a draw into four GPU-ready vertices: backend-free.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "Renderer/QuadBuilder/QuadBuilder.hpp"

#include <cmath>
#include <utility>

namespace gfx {

    namespace {
        Quad corners(const Rect& dest, const float rotation, const Rect& uv, const Color color)
        {
            const float halfW = dest.w * 0.5f;
            const float halfH = dest.h * 0.5f;
            const float centerX = dest.x + halfW;
            const float centerY = dest.y + halfH;
            const float cosine = std::cos(rotation);
            const float sine = std::sin(rotation);

            const std::array<Vec2, 4> offsets{ { { -halfW, -halfH }, { halfW, -halfH }, { halfW, halfH }, { -halfW, halfH } } };
            const std::array<Vec2, 4> uvs{ { { uv.x, uv.y }, { uv.w, uv.y }, { uv.w, uv.h }, { uv.x, uv.h } } };

            Quad quad;
            for (size_t i = 0; i < quad.size(); ++i) {
                // y points down, so the textbook rotation turns clockwise on screen, like SDL.
                quad[i].x = centerX + offsets[i].x * cosine - offsets[i].y * sine;
                quad[i].y = centerY + offsets[i].x * sine + offsets[i].y * cosine;
                quad[i].u = uvs[i].x;
                quad[i].v = uvs[i].y;
                quad[i].r = color.r;
                quad[i].g = color.g;
                quad[i].b = color.b;
                quad[i].a = color.a;
            }

            return quad;
        }
    }

    Quad buildQuad(const SpriteDraw& draw, const Vec2 textureSize)
    {
        // Rect reused as (u0, v0, u1, v1).
        Rect uv{ 0.0f, 0.0f, 1.0f, 1.0f };

        const bool wholeTexture = draw.source.w <= 0.0f || draw.source.h <= 0.0f;
        if (!wholeTexture && textureSize.x > 0.0f && textureSize.y > 0.0f) {
            uv = Rect{
                draw.source.x / textureSize.x,
                draw.source.y / textureSize.y,
                (draw.source.x + draw.source.w) / textureSize.x,
                (draw.source.y + draw.source.h) / textureSize.y
            };
        }

        if (draw.flipX) {
            std::swap(uv.x, uv.w);
        }

        return corners(draw.dest, draw.rotation, uv, draw.tint);
    }

    Quad buildQuad(const RectDraw& draw)
    {
        return corners(draw.dest, 0.0f, Rect{ 0.0f, 0.0f, 1.0f, 1.0f }, draw.color);
    }

}
