/*
    Created on: 11/21/26
    Filename: ViewMapping.hpp
    Description: Where the camera puts world points in a panel the renderer fills 1:1.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#pragma once

#include "Renderer/RendererTypes.hpp"

// screen = origin + world * scale, the same mapping as gfx::worldToScreen, moved to the panel's top-left.
struct ViewMapping
{
    gfx::Vec2 origin;
    float scale = 1.0f;
};

// For a renderer of the panel's size whose image is drawn over the panel at topLeft.
ViewMapping viewMapping(const gfx::Camera2D &camera, gfx::Vec2 topLeft, gfx::Vec2 size);
