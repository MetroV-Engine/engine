/*
    Created on: 9/22/26
    Filename: Transform.hpp
    Description: World/screen conversions shared by every backend, so they cannot drift apart.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#ifndef RENDERER_WRAPPER_TRANSFORM_HPP
#define RENDERER_WRAPPER_TRANSFORM_HPP

#include "Renderer/RendererTypes.hpp"

namespace gfx {

    Vec2 worldToScreen(const Camera2D& camera, Vec2 world, Vec2 viewportSize);
    Rect worldToScreen(const Camera2D& camera, const Rect& world, Vec2 viewportSize);

    // The editor needs this for mouse picking inside the viewport.
    Vec2 screenToWorld(const Camera2D& camera, Vec2 screen, Vec2 viewportSize);

}

#endif //RENDERER_WRAPPER_TRANSFORM_HPP
