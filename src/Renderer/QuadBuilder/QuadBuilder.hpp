/*
    Created on: 10/4/26
    Filename: QuadBuilder.hpp
    Description: Turns a draw into four GPU-ready vertices: backend-free.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#ifndef RENDERER_WRAPPER_QUADBUILDER_HPP
#define RENDERER_WRAPPER_QUADBUILDER_HPP

#include "Renderer/RendererTypes.hpp"

#include <array>
#include <cstdint>

namespace gfx {

    // Corners TL, TR, BR, BL of dest before rotation; positions stay in the draw's space.
    using Quad = std::array<Vertex, 4>;

    inline constexpr std::array<uint16_t, 6> QuadIndices{ 0, 1, 2, 0, 2, 3 };

    Quad buildQuad(const SpriteDraw& draw, Vec2 textureSize);

    // UVs span 0..1 so the quad samples the renderer's 1x1 white texture.
    Quad buildQuad(const RectDraw& draw);

}

#endif //RENDERER_WRAPPER_QUADBUILDER_HPP
