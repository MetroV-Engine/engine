#include <boost/ut.hpp>

#include "Support/Expect.hpp"
#include "Renderer/NullRenderer/NullRenderer.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

using namespace boost::ut;

namespace {

    using Kind = gfx::NullRenderer::Recorded::Kind;

    std::vector<uint8_t> pixels(const uint32_t width, const uint32_t height)
    {
        return std::vector<uint8_t>(static_cast<size_t>(width) * height * 4, 255);
    }

    gfx::RectDraw someRect()
    {
        return gfx::RectDraw{ gfx::Rect{ 1.0f, 2.0f, 3.0f, 4.0f }, gfx::Color{ 10, 20, 30, 40 }, gfx::Layer::Foreground, gfx::Space::Screen };
    }

    std::vector<gfx::Vertex> threeVertices()
    {
        std::vector<gfx::Vertex> vertices(3);
        vertices[1].x = 10.0f;
        vertices[2].y = 10.0f;
        return vertices;
    }

    gfx::MeshDraw meshOf(const std::vector<gfx::Vertex>& vertices, const std::vector<uint16_t>& indices)
    {
        gfx::MeshDraw draw;
        draw.vertices = vertices;
        draw.indices = indices;
        return draw;
    }

}

void run_null_renderer_tests()
{
    "null renderer: layer draws background then game then foreground"_test = [] {
        expect(gfx::layerSlot(gfx::Layer::Background) == 0);
        expect(gfx::layerSlot(gfx::Layer::Game) == 1);
        expect(gfx::layerSlot(gfx::Layer::Foreground) == 2);
    };

    "null renderer: layer invalid value falls back to game"_test = [] {
        expect((gfx::layerSlot(static_cast<gfx::Layer>(99))) == 1);
    };

    "null renderer: layer draws default to game"_test = [] {
        expect((gfx::RectDraw{}.layer) == gfx::Layer::Game);
        expect((gfx::SpriteDraw{}.layer) == gfx::Layer::Game);
    };

    "null renderer: camera starts centered on viewport"_test = [] {
        const gfx::NullRenderer renderer(800, 600);

        expect(test::floatEq(renderer.camera().center.x, 400.0f));
        expect(test::floatEq(renderer.camera().center.y, 300.0f));
        expect(test::floatEq(renderer.camera().zoom, 1.0f));
    };

    "null renderer: records draws only inside frame"_test = [] {
        gfx::NullRenderer renderer(800, 600);

        renderer.drawRect(someRect());
        renderer.beginFrame(gfx::Black);
        renderer.drawRect(someRect());
        renderer.drawSprite(gfx::SpriteDraw{});
        renderer.endFrame();
        renderer.drawSprite(gfx::SpriteDraw{});

        expect(renderer.recorded().size() == 2u) << fatal;
        expect(renderer.recorded()[0].kind == Kind::Rect);
        expect(renderer.recorded()[1].kind == Kind::Sprite);
    };

    "null renderer: keeps draw payloads"_test = [] {
        gfx::NullRenderer renderer(800, 600);

        gfx::SpriteDraw sprite;
        sprite.texture = static_cast<gfx::TextureHandle>(7);
        sprite.source = gfx::Rect{ 512.0f, 0.0f, 512.0f, 512.0f };
        sprite.rotation = 1.5f;
        sprite.flipX = true;
        sprite.layer = gfx::Layer::Background;
        sprite.tint = gfx::Color{ 1, 2, 3, 4 };

        renderer.beginFrame(gfx::Black);
        renderer.drawRect(someRect());
        renderer.drawSprite(sprite);
        renderer.endFrame();

        expect(renderer.recorded().size() == 2u) << fatal;

        const gfx::RectDraw& rect = renderer.recorded()[0].rect;
        expect(test::floatEq(rect.dest.w, 3.0f));
        expect(rect.color.a == 40);
        expect(rect.layer == gfx::Layer::Foreground);
        expect(rect.space == gfx::Space::Screen);

        const gfx::SpriteDraw& recorded = renderer.recorded()[1].sprite;
        expect(recorded.texture == sprite.texture);
        expect(test::floatEq(recorded.source.x, 512.0f));
        expect(test::floatEq(recorded.rotation, 1.5f));
        expect(recorded.flipX);
        expect(recorded.layer == gfx::Layer::Background);
        expect(recorded.tint.b == 3);
    };

    "null renderer: counts frames and remembers clear color"_test = [] {
        gfx::NullRenderer renderer(800, 600);

        renderer.beginFrame(gfx::Color{ 24, 26, 32, 255 });
        renderer.endFrame();
        renderer.beginFrame(gfx::White);
        renderer.endFrame();

        expect(renderer.framesPresented() == 2u);
        expect(renderer.lastClear().r == 255);
    };

    "null renderer: stores camera"_test = [] {
        gfx::NullRenderer renderer(800, 600);

        gfx::Camera2D camera;
        camera.center = gfx::Vec2{ -12.0f, 34.0f };
        camera.zoom = 3.0f;
        renderer.setCamera(camera);

        expect(test::floatEq(renderer.camera().center.x, -12.0f));
        expect(test::floatEq(renderer.camera().center.y, 34.0f));
        expect(test::floatEq(renderer.camera().zoom, 3.0f));
    };

    "null renderer: creates distinct textures with their size"_test = [] {
        gfx::NullRenderer renderer(800, 600);
        const std::vector<uint8_t> wide = pixels(4, 1);
        const std::vector<uint8_t> tall = pixels(2, 3);

        const gfx::TextureHandle first = renderer.createTexture(wide.data(), 4, 1);
        const gfx::TextureHandle second = renderer.createTexture(tall.data(), 2, 3);

        expect(first != gfx::TextureHandle::Invalid);
        expect(second != gfx::TextureHandle::Invalid);
        expect(first != second);
        expect(test::floatEq(renderer.textureSize(first).x, 4.0f));
        expect(test::floatEq(renderer.textureSize(first).y, 1.0f));
        expect(test::floatEq(renderer.textureSize(second).x, 2.0f));
        expect(test::floatEq(renderer.textureSize(second).y, 3.0f));
    };

    "null renderer: rejects empty texture input"_test = [] {
        gfx::NullRenderer renderer(800, 600);
        const std::vector<uint8_t> data = pixels(2, 2);

        expect((renderer.createTexture(nullptr, 2, 2)) == gfx::TextureHandle::Invalid);
        expect((renderer.createTexture(data.data(), 0, 2)) == gfx::TextureHandle::Invalid);
        expect((renderer.createTexture(data.data(), 2, 0)) == gfx::TextureHandle::Invalid);
    };

    "null renderer: destroyed or unknown texture has zero size"_test = [] {
        gfx::NullRenderer renderer(800, 600);
        const std::vector<uint8_t> data = pixels(2, 2);
        const gfx::TextureHandle texture = renderer.createTexture(data.data(), 2, 2);

        renderer.destroyTexture(texture);
        renderer.destroyTexture(texture);                       // twice: must be harmless
        renderer.destroyTexture(gfx::TextureHandle::Invalid);

        expect(test::floatEq(renderer.textureSize(texture).x, 0.0f));
        expect(test::floatEq(renderer.textureSize(gfx::TextureHandle::Invalid).y, 0.0f));
    };

    "null renderer: clear recorded empties the log"_test = [] {
        gfx::NullRenderer renderer(800, 600);

        renderer.beginFrame(gfx::Black);
        renderer.drawRect(someRect());
        renderer.endFrame();
        renderer.clearRecorded();

        expect(renderer.recorded().empty());
    };

    "null renderer: every shape is recorded with its kind"_test = [] {
        gfx::NullRenderer renderer(800, 600);
        const std::array<gfx::Vec2, 3> points{ gfx::Vec2{ 0.0f, 0.0f }, gfx::Vec2{ 4.0f, 0.0f }, gfx::Vec2{ 0.0f, 4.0f } };
        const std::vector<gfx::Vertex> vertices = threeVertices();
        const std::vector<uint16_t> indices{ 0, 1, 2 };

        gfx::LineDraw line;
        line.thickness = 2.5f;
        gfx::TriangleDraw triangle;
        triangle.points = points;
        gfx::QuadDraw quad;
        quad.corners[2] = gfx::Vec2{ 7.0f, 8.0f };
        gfx::CircleDraw circle;
        circle.radius = 9.0f;
        circle.segments = 12;
        gfx::PolygonDraw polygon;
        polygon.points = points;
        gfx::TextDraw text;
        text.text = "hi";
        text.position = gfx::Vec2{ 3.0f, 6.0f };

        renderer.beginFrame(gfx::Black);
        renderer.drawLine(line);
        renderer.drawTriangle(triangle);
        renderer.drawQuad(quad);
        renderer.drawCircle(circle);
        renderer.drawPolygon(polygon);
        renderer.drawMesh(meshOf(vertices, indices));
        renderer.drawText(text);
        renderer.endFrame();

        const std::vector<gfx::NullRenderer::Recorded>& recorded = renderer.recorded();
        expect(recorded.size() == 7u) << fatal;
        expect(recorded[0].kind == Kind::Line);
        expect(recorded[1].kind == Kind::Triangle);
        expect(recorded[2].kind == Kind::Quad);
        expect(recorded[3].kind == Kind::Circle);
        expect(recorded[4].kind == Kind::Polygon);
        expect(recorded[5].kind == Kind::Mesh);
        expect(recorded[6].kind == Kind::Text);

        expect(test::floatEq(recorded[0].line.thickness, 2.5f));
        expect(test::floatEq(recorded[1].triangle.points[1].x, 4.0f));
        expect(test::floatEq(recorded[2].quad.corners[2].y, 8.0f));
        expect(test::floatEq(recorded[3].circle.radius, 9.0f));
        expect(recorded[3].circle.segments == 12);
        expect(recorded[4].polygonPoints.size() == 3u);
        expect(recorded[5].meshVertices.size() == 3u);
        expect(recorded[6].textContent == ("hi"));
        expect(test::floatEq(recorded[6].text.position.y, 6.0f));
    };

    "null renderer: polygon keeps its own points"_test = [] {
        gfx::NullRenderer renderer(800, 600);
        std::vector<gfx::Vec2> points{ gfx::Vec2{ 1.0f, 2.0f }, gfx::Vec2{ 5.0f, 2.0f }, gfx::Vec2{ 5.0f, 6.0f } };

        gfx::PolygonDraw polygon;
        polygon.points = points;

        renderer.beginFrame(gfx::Black);
        renderer.drawPolygon(polygon);
        renderer.endFrame();

        points[0].x = 99.0f;
        points.clear();

        expect(renderer.recorded().size() == 1u) << fatal;
        const std::vector<gfx::Vec2>& kept = renderer.recorded()[0].polygonPoints;
        expect(kept.size() == 3u) << fatal;
        expect(test::floatEq(kept[0].x, 1.0f));
        expect(test::floatEq(kept[2].y, 6.0f));
    };

    "null renderer: text keeps its own string"_test = [] {
        gfx::NullRenderer renderer(800, 600);
        const gfx::Font* const font = reinterpret_cast<const gfx::Font*>(std::uintptr_t{ 0x1000 });

        renderer.beginFrame(gfx::Black);
        {
            const std::string scoped = "scoped text";
            gfx::TextDraw draw;
            draw.font = font;
            draw.text = scoped;
            renderer.drawText(draw);
        }
        renderer.endFrame();

        expect(renderer.recorded().size() == 1u) << fatal;
        expect(renderer.recorded()[0].textContent == ("scoped text"));
        expect(renderer.recorded()[0].text.font == font);
    };

    "null renderer: shapes outside frame are ignored"_test = [] {
        gfx::NullRenderer renderer(800, 600);
        const std::array<gfx::Vec2, 3> points{ gfx::Vec2{ 0.0f, 0.0f }, gfx::Vec2{ 4.0f, 0.0f }, gfx::Vec2{ 0.0f, 4.0f } };

        gfx::PolygonDraw polygon;
        polygon.points = points;
        gfx::TextDraw text;
        text.text = "hi";

        renderer.drawLine(gfx::LineDraw{});
        renderer.drawTriangle(gfx::TriangleDraw{});
        renderer.drawQuad(gfx::QuadDraw{});
        renderer.drawCircle(gfx::CircleDraw{});
        renderer.drawPolygon(polygon);
        renderer.drawText(text);
        renderer.beginFrame(gfx::Black);
        renderer.endFrame();
        renderer.drawLine(gfx::LineDraw{});
        renderer.drawText(text);

        expect(renderer.recorded().empty());
    };

    "null renderer: mesh keeps its own copy"_test = [] {
        gfx::NullRenderer renderer(800, 600);
        std::vector<gfx::Vertex> vertices = threeVertices();
        std::vector<uint16_t> indices{ 0, 1, 2 };

        renderer.beginFrame(gfx::Black);
        renderer.drawMesh(meshOf(vertices, indices));
        renderer.endFrame();

        vertices[1].x = 99.0f;
        indices[2] = 0;
        vertices.clear();
        indices.clear();

        expect(renderer.recorded().size() == 1u) << fatal;

        const gfx::NullRenderer::Recorded& entry = renderer.recorded()[0];
        expect(entry.meshVertices.size() == 3u) << fatal;
        expect(entry.meshIndices.size() == 3u) << fatal;
        expect(test::floatEq(entry.meshVertices[1].x, 10.0f));
        expect(entry.meshIndices[2] == 2);
    };

    "null renderer: rejects malformed mesh"_test = [] {
        gfx::NullRenderer renderer(800, 600);
        const std::vector<gfx::Vertex> vertices = threeVertices();
        const std::vector<uint16_t> twoIndices{ 0, 1 };
        const std::vector<uint16_t> pastTheEnd{ 0, 1, 3 };

        renderer.beginFrame(gfx::Black);
        renderer.drawMesh(meshOf(vertices, twoIndices));
        renderer.drawMesh(meshOf(vertices, pastTheEnd));
        renderer.endFrame();

        expect(renderer.recorded().empty());
    };

    "null renderer: rejects unknown mesh texture"_test = [] {
        gfx::NullRenderer renderer(800, 600);
        const std::vector<gfx::Vertex> vertices = threeVertices();
        const std::vector<uint16_t> indices{ 0, 1, 2 };

        gfx::MeshDraw draw = meshOf(vertices, indices);
        draw.texture = static_cast<gfx::TextureHandle>(999);

        renderer.beginFrame(gfx::Black);
        renderer.drawMesh(draw);
        expect(renderer.recorded().empty());

        draw.texture = gfx::TextureHandle::Invalid;
        renderer.drawMesh(draw);
        renderer.endFrame();

        expect(renderer.recorded().size() == 1u) << fatal;
        expect(renderer.recorded()[0].meshTexture == gfx::TextureHandle::Invalid);
    };

    "null renderer: empty mesh records nothing"_test = [] {
        gfx::NullRenderer renderer(800, 600);
        const std::vector<gfx::Vertex> vertices = threeVertices();
        const std::vector<uint16_t> indices{ 0, 1, 2 };

        renderer.beginFrame(gfx::Black);
        renderer.drawMesh(gfx::MeshDraw{});
        renderer.drawMesh(meshOf(vertices, {}));
        renderer.drawMesh(meshOf({}, indices));
        renderer.endFrame();

        expect(renderer.recorded().empty());
    };

    "null renderer: mesh outside frame is ignored"_test = [] {
        gfx::NullRenderer renderer(800, 600);
        const std::vector<gfx::Vertex> vertices = threeVertices();
        const std::vector<uint16_t> indices{ 0, 1, 2 };

        renderer.drawMesh(meshOf(vertices, indices));
        renderer.beginFrame(gfx::Black);
        renderer.endFrame();
        renderer.drawMesh(meshOf(vertices, indices));

        expect(renderer.recorded().empty());
    };
}
