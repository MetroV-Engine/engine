/*
    Created on: 9/22/26
    Filename: Transform.cpp
    Description: World/screen conversions shared by every backend, so they cannot drift apart.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "Renderer/Transform/Transform.hpp"

namespace gfx {

    namespace {
        // A zoom of 0 would divide by zero in screenToWorld and collapse the world in the other
        // direction; treat it as the identity rather than propagating infinities.
        float safeZoom(const float zoom)
        {
            return zoom > 0.0f ? zoom : 1.0f;
        }
    }

    Vec2 worldToScreen(const Camera2D& camera, const Vec2 world, const Vec2 viewportSize)
    {
        const float zoom = safeZoom(camera.zoom);

        return Vec2{
            (world.x - camera.center.x) * zoom + viewportSize.x * 0.5f,
            (world.y - camera.center.y) * zoom + viewportSize.y * 0.5f
        };
    }

    Rect worldToScreen(const Camera2D& camera, const Rect& world, const Vec2 viewportSize)
    {
        const Vec2 topLeft = worldToScreen(camera, Vec2{ world.x, world.y }, viewportSize);
        const float zoom = safeZoom(camera.zoom);

        return Rect{ topLeft.x, topLeft.y, world.w * zoom, world.h * zoom };
    }

    Vec2 screenToWorld(const Camera2D& camera, const Vec2 screen, const Vec2 viewportSize)
    {
        const float zoom = safeZoom(camera.zoom);

        return Vec2{
            (screen.x - viewportSize.x * 0.5f) / zoom + camera.center.x,
            (screen.y - viewportSize.y * 0.5f) / zoom + camera.center.y
        };
    }

}
