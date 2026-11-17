/*
    Created on: 11/9/26
    Filename: ShapeBuilder.cpp
    Description: Shape tessellation: lines, circles and ear-clipped polygons into triangles.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "Renderer/ShapeBuilder/ShapeBuilder.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace gfx {

    namespace {

        constexpr int MinAutoSegments = 12;
        constexpr int MaxAutoSegments = 128;
        constexpr int MinSegments = 3;
        constexpr int MaxSegments = 1024;
        // Largest allowed gap between the circle and its chord, in the circle's own units.
        constexpr double CircleTolerance = 0.25;

        Vertex makeVertex(const Vec2 p, const Color c)
        {
            Vertex v;
            v.x = p.x;
            v.y = p.y;
            v.u = 0.0f;
            v.v = 0.0f;
            v.r = c.r;
            v.g = c.g;
            v.b = c.b;
            v.a = c.a;
            return v;
        }

        uint16_t baseIndex(const TriangleMesh& mesh)
        {
            return static_cast<uint16_t>(mesh.vertices.size());
        }

        void pushQuadIndices(TriangleMesh& mesh, const uint16_t base)
        {
            for (const int i : { 0, 1, 2, 0, 2, 3 }) {
                mesh.indices.push_back(static_cast<uint16_t>(base + i));
            }
        }

        int autoSegments(const double radius)
        {
            const double ratio = std::clamp(1.0 - CircleTolerance / radius, -1.0, 1.0);
            const double step = std::acos(ratio);
            const double count = step > 0.0 ? std::ceil(2.0 * std::numbers::pi / step) : MaxAutoSegments;
            return static_cast<int>(std::clamp(count, static_cast<double>(MinAutoSegments),
                                               static_cast<double>(MaxAutoSegments)));
        }

        double cross(const Vec2 a, const Vec2 b, const Vec2 c)
        {
            return (static_cast<double>(b.x) - a.x) * (static_cast<double>(c.y) - a.y)
                 - (static_cast<double>(b.y) - a.y) * (static_cast<double>(c.x) - a.x);
        }

        bool samePoint(const Vec2 a, const Vec2 b)
        {
            return a.x == b.x && a.y == b.y;
        }

        // a, b, c wind positively. A point sharing a corner's position never blocks: duplicates are tolerated.
        bool insideTriangle(const Vec2 a, const Vec2 b, const Vec2 c, const Vec2 p)
        {
            if (samePoint(p, a) || samePoint(p, b) || samePoint(p, c)) return false;
            return cross(a, b, p) >= 0.0 && cross(b, c, p) >= 0.0 && cross(c, a, p) >= 0.0;
        }

        double signedArea(const std::span<const Vec2> points)
        {
            double sum = 0.0;
            for (size_t i = 0; i < points.size(); ++i) {
                const Vec2 a = points[i];
                const Vec2 b = points[(i + 1) % points.size()];
                sum += static_cast<double>(a.x) * b.y - static_cast<double>(b.x) * a.y;
            }
            return sum * 0.5;
        }

    }

    void TriangleMesh::clear()
    {
        vertices.clear();
        indices.clear();
    }

    void appendLine(TriangleMesh& mesh, const LineDraw& draw)
    {
        const double dx = static_cast<double>(draw.to.x) - draw.from.x;
        const double dy = static_cast<double>(draw.to.y) - draw.from.y;
        const double length = std::hypot(dx, dy);
        if (!(length > 0.0) || !(draw.thickness > 0.0f)) return;

        const double half = static_cast<double>(draw.thickness) * 0.5;
        const Vec2 n{ static_cast<float>(-dy / length * half), static_cast<float>(dx / length * half) };

        const uint16_t base = baseIndex(mesh);
        mesh.vertices.push_back(makeVertex({ draw.from.x + n.x, draw.from.y + n.y }, draw.color));
        mesh.vertices.push_back(makeVertex({ draw.to.x + n.x, draw.to.y + n.y }, draw.color));
        mesh.vertices.push_back(makeVertex({ draw.to.x - n.x, draw.to.y - n.y }, draw.color));
        mesh.vertices.push_back(makeVertex({ draw.from.x - n.x, draw.from.y - n.y }, draw.color));
        pushQuadIndices(mesh, base);
    }

    void appendTriangle(TriangleMesh& mesh, const TriangleDraw& draw)
    {
        const uint16_t base = baseIndex(mesh);
        for (const Vec2 p : draw.points) mesh.vertices.push_back(makeVertex(p, draw.color));
        for (uint16_t i = 0; i < 3; ++i) mesh.indices.push_back(static_cast<uint16_t>(base + i));
    }

    void appendQuad(TriangleMesh& mesh, const QuadDraw& draw)
    {
        const uint16_t base = baseIndex(mesh);
        for (const Vec2 p : draw.corners) mesh.vertices.push_back(makeVertex(p, draw.color));
        pushQuadIndices(mesh, base);
    }

    void appendCircle(TriangleMesh& mesh, const CircleDraw& draw)
    {
        if (!(draw.radius > 0.0f)) return;

        const int segments = draw.segments <= 0
            ? autoSegments(draw.radius)
            : std::clamp(draw.segments, MinSegments, MaxSegments);

        const uint16_t base = baseIndex(mesh);
        mesh.vertices.push_back(makeVertex(draw.center, draw.color));
        for (int k = 0; k < segments; ++k) {
            const double angle = 2.0 * std::numbers::pi * k / segments;
            const Vec2 p{ static_cast<float>(draw.center.x + draw.radius * std::cos(angle)),
                          static_cast<float>(draw.center.y + draw.radius * std::sin(angle)) };
            mesh.vertices.push_back(makeVertex(p, draw.color));
        }
        for (int k = 0; k < segments; ++k) {
            mesh.indices.push_back(base);
            mesh.indices.push_back(static_cast<uint16_t>(base + 1 + k));
            mesh.indices.push_back(static_cast<uint16_t>(base + 1 + (k + 1) % segments));
        }
    }

    bool appendPolygon(TriangleMesh& mesh, const PolygonDraw& draw)
    {
        const std::span<const Vec2> points = draw.points;
        const size_t count = points.size();
        if (count < 3 || count > MaxMeshVertices) return false;
        if (mesh.vertices.size() + count > MaxMeshVertices) return false;

        const double area = signedArea(points);
        if (!(std::abs(area) > 0.0)) return false;

        const size_t vertexStart = mesh.vertices.size();
        const size_t indexStart = mesh.indices.size();
        const uint16_t base = baseIndex(mesh);
        for (const Vec2 p : points) mesh.vertices.push_back(makeVertex(p, draw.color));

        std::vector<size_t> remaining(count);
        for (size_t i = 0; i < count; ++i) remaining[i] = area > 0.0 ? i : count - 1 - i;

        const auto emit = [&](const size_t a, const size_t b, const size_t c) {
            mesh.indices.push_back(static_cast<uint16_t>(base + a));
            mesh.indices.push_back(static_cast<uint16_t>(base + b));
            mesh.indices.push_back(static_cast<uint16_t>(base + c));
        };

        while (remaining.size() > 3) {
            bool removed = false;
            const size_t n = remaining.size();
            for (size_t i = 0; i < n && !removed; ++i) {
                const size_t ip = remaining[(i + n - 1) % n];
                const size_t ic = remaining[i];
                const size_t in = remaining[(i + 1) % n];
                const Vec2 a = points[ip];
                const Vec2 b = points[ic];
                const Vec2 c = points[in];
                const double turn = cross(a, b, c);

                if (turn == 0.0) {
                    remaining.erase(remaining.begin() + static_cast<std::ptrdiff_t>(i));
                    removed = true;
                } else if (turn > 0.0) {
                    bool blocked = false;
                    for (const size_t other : remaining) {
                        if (other == ip || other == ic || other == in) continue;
                        if (insideTriangle(a, b, c, points[other])) { blocked = true; break; }
                    }
                    if (!blocked) {
                        emit(ip, ic, in);
                        remaining.erase(remaining.begin() + static_cast<std::ptrdiff_t>(i));
                        removed = true;
                    }
                }
            }
            if (!removed) {
                mesh.vertices.resize(vertexStart);
                mesh.indices.resize(indexStart);
                return false;
            }
        }

        if (remaining.size() == 3
            && cross(points[remaining[0]], points[remaining[1]], points[remaining[2]]) != 0.0) {
            emit(remaining[0], remaining[1], remaining[2]);
        }
        return true;
    }

    bool isValidMesh(const std::span<const Vertex> vertices, const std::span<const uint16_t> indices)
    {
        if (vertices.size() > MaxMeshVertices) return false;
        if (indices.size() % 3 != 0) return false;
        return std::all_of(indices.begin(), indices.end(),
                           [&](const uint16_t i) { return i < vertices.size(); });
    }

}
