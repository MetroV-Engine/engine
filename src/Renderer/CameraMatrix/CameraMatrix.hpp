/*
    Created on: 10/4/26
    Filename: CameraMatrix.hpp
    Description: Camera as a GPU matrix: the twin of worldToScreen.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#ifndef RENDERER_WRAPPER_CAMERAMATRIX_HPP
#define RENDERER_WRAPPER_CAMERAMATRIX_HPP

#include "Renderer/RendererTypes.hpp"

#include <array>

namespace gfx {

    // Column-major, the layout bgfx::setTransform reads.
    using Mat4 = std::array<float, 16>;

    // World units -> back-buffer pixels; must agree with worldToScreen for the same inputs.
    Mat4 cameraMatrix(const Camera2D& camera, Vec2 viewportSize);

    Mat4 identityMatrix();

}

#endif //RENDERER_WRAPPER_CAMERAMATRIX_HPP
