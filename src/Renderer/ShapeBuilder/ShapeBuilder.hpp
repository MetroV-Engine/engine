/*
    Created on: 11/9/26
    Filename: ShapeBuilder.hpp
    Description: Turns shapes into indexed triangles: backend-free.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#ifndef RENDERER_WRAPPER_SHAPEBUILDER_HPP
#define RENDERER_WRAPPER_SHAPEBUILDER_HPP

#include "Renderer/RendererTypes.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace gfx {

    struct TriangleMesh {
        std::vector<Vertex> vertices;
        std::vector<uint16_t> indices;

        void clear();
    };

    // Indices are 16-bit, so one mesh addresses at most this many vertices.
    inline constexpr size_t MaxMeshVertices = 65536;

    // The append functions add to mesh; zero-length lines and non-positive radii add nothing.
    void appendLine(TriangleMesh& mesh, const LineDraw& draw);
    void appendTriangle(TriangleMesh& mesh, const TriangleDraw& draw);
    void appendQuad(TriangleMesh& mesh, const QuadDraw& draw);
    void appendCircle(TriangleMesh& mesh, const CircleDraw& draw);

    // Ear clipping; false, with the mesh unchanged, if ear clipping cannot finish or for degenerate or oversized input.
    bool appendPolygon(TriangleMesh& mesh, const PolygonDraw& draw);

    bool isValidMesh(std::span<const Vertex> vertices, std::span<const uint16_t> indices);

}

#endif //RENDERER_WRAPPER_SHAPEBUILDER_HPP
