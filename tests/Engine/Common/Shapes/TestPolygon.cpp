#include <boost/ut.hpp>

#include <cmath>

#include "Engine/Common/Shapes/Polygon.hpp"
#include "../../Support/Approx.hpp"

using namespace boost::ut;
using namespace Engine::Shapes;
using TestSupport::near;

namespace {
// A slope rising to the right: its surface is the line y = x, from (0, 0) to (2, 2).
constexpr Polygon Slope{{0.f, 0.f}, {2.f, 0.f}, {2.f, 2.f}};

void test_construction() {
    "a Polygon keeps its vertices in order, and ignores those past MaxVertices"_test = [] {
        expect(constant<Slope.count == 3>);
        expect(constant<Slope.vertex(1) == Vector2f(2.f, 0.f)>);
        expect(constant<Slope.vertex(3) == Vector2f(0.f, 0.f)>) << "vertex(count) wraps to the first";
        expect(constant<Slope.edge(2) == Vector2f(-2.f, -2.f)>);

        constexpr Polygon tooMany{{0.f, 0.f}, {1.f, 0.f}, {2.f, 1.f}, {3.f, 3.f}, {3.f, 4.f},
                                  {2.f, 5.f}, {1.f, 5.f}, {0.f, 4.f}, {-1.f, 3.f}};
        expect(constant<tooMany.count == Polygon::MaxVertices>);
    };

    "fromRectangle gives its four corners, counter-clockwise from min"_test = [] {
        constexpr Polygon box = Polygon::fromRectangle({{1.f, 2.f}, {3.f, 5.f}});

        expect(constant<box == Polygon{{1.f, 2.f}, {3.f, 2.f}, {3.f, 5.f}, {1.f, 5.f}}>);
        expect(constant<box.isConvex()>);
    };

    "isConvex accepts convex counter-clockwise polygons only"_test = [] {
        expect(constant<Slope.isConvex()>);
        expect(constant<!Polygon{{0.f, 0.f}, {2.f, 2.f}, {2.f, 0.f}}.isConvex()>) << "clockwise";
        expect(constant<!Polygon{{0.f, 0.f}, {1.f, 0.f}, {2.f, 0.f}, {1.f, 1.f}}.isConvex()>) << "three aligned";
        expect(constant<!Polygon{{0.f, 0.f}, {2.f, 0.f}, {1.f, 0.5f}, {2.f, 2.f}, {0.f, 2.f}}.isConvex()>) << "concave";
        expect(constant<!Polygon{{0.f, 0.f}, {1.f, 0.f}}.isConvex()>) << "two vertices";
        // A pentagram turns left at every vertex but winds twice.
        Polygon star;
        for (int i = 0; i < 5; ++i) {
            const double angle = 6.283185307179586 * (2 * i) / 5;
            star.vertices[star.count++] = {static_cast<float>(std::cos(angle)), static_cast<float>(std::sin(angle))};
        }
        expect(!star.isConvex()) << "self-crossing star";
    };
}

void test_geometry() {
    "normals point out, with length 1, exactly on axis-aligned edges"_test = [] {
        expect(Slope.normal(0) == Vector2f(0.f, -1.f));
        expect(Slope.normal(1) == Vector2f(1.f, 0.f));
        const float diagonal = std::sqrt(0.5f);
        expect(near(Slope.normal(2), Vector2f(-diagonal, diagonal), 1e-6f));
    };

    "center, bounding box, translated and support"_test = [] {
        expect(constant<Slope.boundingBox() == Rectangle{{0.f, 0.f}, {2.f, 2.f}}>);
        expect(near(Slope.center(), Vector2f(4.f / 3.f, 2.f / 3.f), 1e-6f));
        expect(constant<Slope.translated({1.f, -1.f}) == Polygon{{1.f, -1.f}, {3.f, -1.f}, {3.f, 1.f}}>);
        expect(constant<Slope.support({-1.f, 1.f}) == Vector2f(0.f, 0.f)>) << "first of the tied vertices";
        expect(constant<Slope.support({1.f, 1.f}) == Vector2f(2.f, 2.f)>);
    };

    "contains includes the edges; closestPoint is the point itself inside, on the boundary outside"_test = [] {
        expect(constant<Slope.contains({1.5f, 0.5f})>);
        expect(constant<Slope.contains({1.f, 1.f})>) << "on the slope";
        expect(constant<!Slope.contains({0.5f, 1.5f})>) << "above the slope";

        expect(constant<Slope.closestPoint({1.5f, 0.5f}) == Vector2f(1.5f, 0.5f)>);
        expect(constant<Slope.closestPoint({0.f, 2.f}) == Vector2f(1.f, 1.f)>) << "foot on the slope";
        expect(constant<Slope.closestPoint({3.f, 3.f}) == Vector2f(2.f, 2.f)>) << "a vertex";
        expect(constant<Slope.closestPoint({1.f, -1.f}) == Vector2f(1.f, 0.f)>) << "the bottom";
    };
}
}

void run_polygon_tests() {
    test_construction();
    test_geometry();
}
