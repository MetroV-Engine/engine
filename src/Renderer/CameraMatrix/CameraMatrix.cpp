/*
    Created on: 10/4/26
    Filename: CameraMatrix.cpp
    Description: Camera as a GPU matrix: the twin of worldToScreen.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "Renderer/CameraMatrix/CameraMatrix.hpp"

namespace gfx {

    Mat4 identityMatrix()
    {
        Mat4 m{};
        m[0] = 1.0f;
        m[5] = 1.0f;
        m[10] = 1.0f;
        m[15] = 1.0f;

        return m;
    }

    Mat4 cameraMatrix(const Camera2D& camera, const Vec2 viewportSize)
    {
        // Same rule as Transform's safeZoom, or the two backends disagree on a zero zoom.
        const float zoom = camera.zoom > 0.0f ? camera.zoom : 1.0f;

        Mat4 m = identityMatrix();
        m[0] = zoom;
        m[5] = zoom;
        m[12] = viewportSize.x * 0.5f - camera.center.x * zoom;
        m[13] = viewportSize.y * 0.5f - camera.center.y * zoom;

        return m;
    }

}
