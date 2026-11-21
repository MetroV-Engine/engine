/*
    Created on: 11/21/26
    Filename: ViewMapping.cpp
    Description: Where the camera puts world points in a panel the renderer fills 1:1.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "Rendering/ViewMapping/ViewMapping.hpp"

ViewMapping viewMapping(const gfx::Camera2D &camera, const gfx::Vec2 topLeft, const gfx::Vec2 size)
{
    // As the renderer does: a zoom of zero or less is drawn as 1.
    const float zoom = camera.zoom > 0.0f ? camera.zoom : 1.0f;
    return ViewMapping{
        gfx::Vec2{topLeft.x + size.x * 0.5f - camera.center.x * zoom, topLeft.y + size.y * 0.5f - camera.center.y * zoom},
        zoom};
}
