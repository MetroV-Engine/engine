#include <boost/ut.hpp>

#include "Support/Expect.hpp"
#include "Renderer/ShapeBuilder/ShapeBuilder.hpp"

#include <array>
#include <cmath>
#include <utility>
#include <vector>

using namespace boost::ut;

using namespace gfx;

namespace {

    double area(const TriangleMesh& mesh)
    {
        double total = 0.0;
        for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
            const Vertex& a = mesh.vertices[mesh.indices[i]];
            const Vertex& b = mesh.vertices[mesh.indices[i + 1]];
            const Vertex& c = mesh.vertices[mesh.indices[i + 2]];
            const double cross = (static_cast<double>(b.x) - a.x) * (static_cast<double>(c.y) - a.y)
                               - (static_cast<double>(b.y) - a.y) * (static_cast<double>(c.x) - a.x);
            total += std::abs(cross) * 0.5;
        }
        return total;
    }

    const Color Tint{ 10, 20, 30, 40 };

    bool hasTint(const Vertex& v)
    {
        return v.r == 10 && v.g == 20 && v.b == 30 && v.a == 40;
    }

    LineDraw lineOf(const float length, const float thickness)
    {
        LineDraw draw;
        draw.from = { 0.0f, 0.0f };
        draw.to = { length, 0.0f };
        draw.thickness = thickness;
        draw.color = Tint;
        return draw;
    }

    CircleDraw circleOf(const float radius, const int segments)
    {
        CircleDraw draw;
        draw.center = { 5.0f, -3.0f };
        draw.radius = radius;
        draw.segments = segments;
        draw.color = Tint;
        return draw;
    }

    TriangleDraw triangleAt(const float x)
    {
        TriangleDraw draw;
        draw.points = { Vec2{ x, 0.0f }, Vec2{ x + 1.0f, 0.0f }, Vec2{ x, 1.0f } };
        draw.color = Tint;
        return draw;
    }

    bool polygon(TriangleMesh& mesh, const std::vector<Vec2>& points)
    {
        PolygonDraw draw;
        draw.points = points;
        draw.color = Tint;
        return appendPolygon(mesh, draw);
    }

    const std::vector<Vec2> Square{ { 0, 0 }, { 10, 0 }, { 10, 10 }, { 0, 10 } };

}

void run_shape_builder_tests()
{
    "shape builder: line is aquad around the segment"_test = [] {
        TriangleMesh mesh;
        appendLine(mesh, lineOf(10.0f, 2.0f));
        expect(mesh.vertices.size() == 4u) << fatal;
        expect(mesh.indices.size() == 6u);
        for (const Vertex& v : mesh.vertices) {
            expect(test::near(std::abs(v.y), 1.0f, 1e-5f));
            expect(hasTint(v));
        }
        expect(test::near(area(mesh), 20.0, 1e-4));
    };

    "shape builder: line ignores zero length and bad thickness"_test = [] {
        TriangleMesh mesh;
        appendLine(mesh, lineOf(0.0f, 2.0f));
        appendLine(mesh, lineOf(10.0f, 0.0f));
        appendLine(mesh, lineOf(10.0f, -1.0f));
        expect(mesh.vertices.empty());
        expect(mesh.indices.empty());
    };

    "shape builder: triangle and quad carry colour"_test = [] {
        TriangleMesh tri;
        TriangleDraw t;
        t.points = { Vec2{ 0, 0 }, Vec2{ 1, 0 }, Vec2{ 0, 1 } };
        t.color = Tint;
        appendTriangle(tri, t);
        expect(tri.vertices.size() == 3u);
        expect(tri.indices.size() == 3u);
        for (const Vertex& v : tri.vertices) expect(hasTint(v));

        TriangleMesh quad;
        QuadDraw q;
        q.corners = { Vec2{ 0, 0 }, Vec2{ 10, 0 }, Vec2{ 10, 10 }, Vec2{ 0, 10 } };
        q.color = Tint;
        appendQuad(quad, q);
        expect(quad.vertices.size() == 4u);
        expect(quad.indices.size() == 6u);
        for (const Vertex& v : quad.vertices) expect(hasTint(v));
        expect(test::near(area(quad), 100.0, 1e-4));
    };

    "shape builder: circle segments from radius"_test = [] {
        TriangleMesh small;
        appendCircle(small, circleOf(1.0f, 0));
        expect(small.vertices.size() == 13u);
        expect(small.indices.size() == 36u);

        TriangleMesh huge;
        appendCircle(huge, circleOf(100000.0f, 0));
        expect(huge.vertices.size() == 129u);
        expect(huge.indices.size() == 384u);
    };

    "shape builder: circle explicit segments are clamped"_test = [] {
        const std::array<std::pair<int, size_t>, 4> cases{ { { 1, 3 }, { 2, 3 }, { 5, 5 }, { 1000000, 1024 } } };
        for (const auto& [requested, expected] : cases) {
            TriangleMesh mesh;
            appendCircle(mesh, circleOf(10.0f, requested));
            expect(mesh.vertices.size() == (expected + 1)) << requested;
            expect(mesh.indices.size() == (expected * 3)) << requested;
        }
    };

    "shape builder: circle ignores non positive radius"_test = [] {
        TriangleMesh mesh;
        appendCircle(mesh, circleOf(0.0f, 0));
        appendCircle(mesh, circleOf(-1.0f, 0));
        expect(mesh.vertices.empty());
        expect(mesh.indices.empty());
    };

    "shape builder: circle rim is at radius"_test = [] {
        TriangleMesh mesh;
        appendCircle(mesh, circleOf(7.0f, 0));
        expect(mesh.vertices.size() > 1u) << fatal;
        for (size_t i = 1; i < mesh.vertices.size(); ++i) {
            const double dx = static_cast<double>(mesh.vertices[i].x) - 5.0;
            const double dy = static_cast<double>(mesh.vertices[i].y) + 3.0;
            expect(test::near(std::hypot(dx, dy), 7.0, 1e-3));
        }
    };

    "shape builder: polygon fills convex"_test = [] {
        TriangleMesh mesh;
        expect(polygon(mesh, Square));
        expect(mesh.vertices.size() == 4u);
        expect(mesh.indices.size() == 6u);
        expect(test::near(area(mesh), 100.0, 1e-4));
        for (const Vertex& v : mesh.vertices) expect(hasTint(v));
    };

    "shape builder: polygon fills concave in both windings"_test = [] {
        std::vector<Vec2> shape{ { 0, 0 }, { 4, 0 }, { 4, 2 }, { 2, 2 }, { 2, 4 }, { 0, 4 } };
        TriangleMesh forward;
        expect(polygon(forward, shape));
        expect(test::near(area(forward), 12.0, 1e-4));

        std::vector<Vec2> reversed(shape.rbegin(), shape.rend());
        TriangleMesh backward;
        expect(polygon(backward, reversed));
        expect(test::near(area(backward), 12.0, 1e-4));
    };

    "shape builder: polygon tolerates duplicates and collinear"_test = [] {
        const std::vector<Vec2> shape{ { 0, 0 }, { 5, 0 }, { 10, 0 }, { 10, 0 }, { 10, 10 }, { 0, 10 } };
        TriangleMesh mesh;
        expect(polygon(mesh, shape));
        expect(test::near(area(mesh), 100.0, 1e-4));
    };

    "shape builder: polygon rejects too few points"_test = [] {
        for (size_t count = 0; count <= 2; ++count) {
            const std::vector<Vec2> points(Square.begin(), Square.begin() + static_cast<std::ptrdiff_t>(count));
            TriangleMesh mesh;
            expect(!(polygon(mesh, points))) << count;
            expect(mesh.vertices.empty());
        }
    };

    "shape builder: polygon failure leaves mesh unchanged"_test = [] {
        TriangleMesh mesh;
        appendTriangle(mesh, triangleAt(0.0f));

        expect(!(polygon(mesh, { { 0, 0 }, { 2, 2 }, { 2, 0 }, { 0, 2 } })));
        expect(mesh.vertices.size() == 3u);
        expect(mesh.indices.size() == 3u);

        expect(!(polygon(mesh, { { 0, 0 }, { 1, 0 }, { 2, 0 }, { 3, 0 } })));
        expect(mesh.vertices.size() == 3u);
        expect(mesh.indices.size() == 3u);

        // Area 0.5: one ear is emitted, then no ear is left, so the rollback path runs.
        expect(!(polygon(mesh, { { 3, 6 }, { 4, 4 }, { 5, 6 }, { 1, 5 }, { 0, 5 } })));
        expect(mesh.vertices.size() == 3u);
        expect(mesh.indices.size() == 3u);
    };

    "shape builder: appends offset indices"_test = [] {
        TriangleMesh mesh;
        appendTriangle(mesh, triangleAt(0.0f));
        appendTriangle(mesh, triangleAt(5.0f));
        expect(mesh.indices.size() == 6u) << fatal;
        expect(mesh.indices[3] == 3);
        expect(mesh.indices[4] == 4);
        expect(mesh.indices[5] == 5);
    };

    "shape builder: is valid mesh"_test = [] {
        const std::vector<Vertex> three(3);
        const std::vector<uint16_t> good{ 0, 1, 2 };
        const std::vector<uint16_t> shortList{ 0, 1 };
        const std::vector<uint16_t> outOfRange{ 0, 1, 3 };
        const std::vector<Vertex> tooMany(MaxMeshVertices + 1);

        expect(isValidMesh({}, {}));
        expect(isValidMesh(three, good));
        expect(!(isValidMesh(three, shortList)));
        expect(!(isValidMesh(three, outOfRange)));
        expect(!(isValidMesh(tooMany, {})));
    };
}
