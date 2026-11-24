#include <boost/ut.hpp>

#include "Support/Expect.hpp"
#include "Renderer/QuadBuilder/QuadBuilder.hpp"

#include <cmath>
#include <numbers>

using namespace boost::ut;

namespace {

    constexpr gfx::Vec2 Sheet{ 4096.0f, 512.0f };
    constexpr size_t TL = 0;
    constexpr size_t TR = 1;
    constexpr size_t BR = 2;
    constexpr size_t BL = 3;

    gfx::SpriteDraw sprite(const gfx::Rect dest, const gfx::Rect source)
    {
        gfx::SpriteDraw draw;
        draw.texture = static_cast<gfx::TextureHandle>(1);
        draw.dest = dest;
        draw.source = source;

        return draw;
    }

}

void run_quad_builder_tests()
{
    "quad builder: axis aligned corners follow dest"_test = [] {
        const gfx::Quad q = gfx::buildQuad(sprite({ 10.0f, 20.0f, 30.0f, 40.0f }, {}), Sheet);

        expect(test::floatEq(q[TL].x, 10.0f)); expect(test::floatEq(q[TL].y, 20.0f));
        expect(test::floatEq(q[TR].x, 40.0f)); expect(test::floatEq(q[TR].y, 20.0f));
        expect(test::floatEq(q[BR].x, 40.0f)); expect(test::floatEq(q[BR].y, 60.0f));
        expect(test::floatEq(q[BL].x, 10.0f)); expect(test::floatEq(q[BL].y, 60.0f));
    };

    "quad builder: source rect becomes normalised uv"_test = [] {
        const gfx::Quad q = gfx::buildQuad(sprite({ 0.0f, 0.0f, 128.0f, 128.0f }, { 1024.0f, 0.0f, 512.0f, 512.0f }), Sheet);

        expect(test::floatEq(q[TL].u, 0.25f));  expect(test::floatEq(q[TL].v, 0.0f));
        expect(test::floatEq(q[BR].u, 0.375f)); expect(test::floatEq(q[BR].v, 1.0f));
        expect(test::floatEq(q[TR].u, 0.375f)); expect(test::floatEq(q[BL].u, 0.25f));
    };

    "quad builder: empty source means whole texture"_test = [] {
        for (const gfx::Rect source : { gfx::Rect{}, gfx::Rect{ 5.0f, 5.0f, -1.0f, 10.0f }, gfx::Rect{ 5.0f, 5.0f, 10.0f, 0.0f } }) {
            const gfx::Quad q = gfx::buildQuad(sprite({ 0.0f, 0.0f, 8.0f, 8.0f }, source), Sheet);

            expect(test::floatEq(q[TL].u, 0.0f)); expect(test::floatEq(q[TL].v, 0.0f));
            expect(test::floatEq(q[BR].u, 1.0f)); expect(test::floatEq(q[BR].v, 1.0f));
        }
    };

    "quad builder: flip xswaps horizontal uv"_test = [] {
        gfx::SpriteDraw draw = sprite({ 0.0f, 0.0f, 128.0f, 128.0f }, { 1024.0f, 0.0f, 512.0f, 512.0f });
        draw.flipX = true;
        const gfx::Quad q = gfx::buildQuad(draw, Sheet);

        expect(test::floatEq(q[TL].u, 0.375f));
        expect(test::floatEq(q[TR].u, 0.25f));
        expect(test::floatEq(q[TL].x, 0.0f));     // positions are not mirrored, only the texture
        expect(test::floatEq(q[TL].v, 0.0f));
    };

    "quad builder: tint is copied to every corner"_test = [] {
        gfx::SpriteDraw draw = sprite({ 0.0f, 0.0f, 8.0f, 8.0f }, {});
        draw.tint = gfx::Color{ 10, 20, 30, 40 };

        for (const gfx::Vertex& v : gfx::buildQuad(draw, Sheet)) {
            expect(v.r == 10); expect(v.g == 20); expect(v.b == 30); expect(v.a == 40);
        }
    };

    "quad builder: quarter turn is clockwise in ydown"_test = [] {
        gfx::SpriteDraw draw = sprite({ 0.0f, 0.0f, 10.0f, 10.0f }, {});
        draw.rotation = std::numbers::pi_v<float> * 0.5f;
        const gfx::Quad q = gfx::buildQuad(draw, Sheet);

        expect(test::near(q[TL].x, 10.0f, 1e-4f)); expect(test::near(q[TL].y, 0.0f, 1e-4f));
        expect(test::near(q[TR].x, 10.0f, 1e-4f)); expect(test::near(q[TR].y, 10.0f, 1e-4f));
        expect(test::near(q[BR].x, 0.0f, 1e-4f));  expect(test::near(q[BR].y, 10.0f, 1e-4f));
        expect(test::near(q[BL].x, 0.0f, 1e-4f));  expect(test::near(q[BL].y, 0.0f, 1e-4f));
    };

    "quad builder: flip and rotation combine"_test = [] {
        gfx::SpriteDraw draw = sprite({ 0.0f, 0.0f, 10.0f, 10.0f }, {});
        draw.rotation = std::numbers::pi_v<float> * 0.5f;
        draw.flipX = true;
        const gfx::Quad q = gfx::buildQuad(draw, Sheet);

        expect(test::near(q[TL].x, 10.0f, 1e-4f)); expect(test::near(q[TL].y, 0.0f, 1e-4f));
        expect(test::floatEq(q[TL].u, 1.0f));
        expect(test::floatEq(q[TR].u, 0.0f));
    };

    "quad builder: rect uses full uv and its color"_test = [] {
        const gfx::RectDraw rect{ gfx::Rect{ 1.0f, 2.0f, 3.0f, 4.0f }, gfx::Color{ 9, 8, 7, 6 }, gfx::Layer::Game, gfx::Space::Screen };
        const gfx::Quad q = gfx::buildQuad(rect);

        expect(test::floatEq(q[TL].x, 1.0f)); expect(test::floatEq(q[BR].y, 6.0f));
        expect(test::floatEq(q[TL].u, 0.0f)); expect(test::floatEq(q[BR].u, 1.0f));
        expect(test::floatEq(q[TL].v, 0.0f)); expect(test::floatEq(q[BR].v, 1.0f));
        for (const gfx::Vertex& v : q) {
            expect(v.r == 9); expect(v.a == 6);
        }
    };

    "quad builder: zero texture size falls back to whole texture"_test = [] {
        const gfx::Quad q = gfx::buildQuad(sprite({ 0.0f, 0.0f, 8.0f, 8.0f }, { 0.0f, 0.0f, 10.0f, 10.0f }), gfx::Vec2{ 0.0f, 0.0f });

        for (const gfx::Vertex& v : q) {
            expect(std::isfinite(v.u) && std::isfinite(v.v));
        }
        expect(test::floatEq(q[TL].u, 0.0f));
        expect(test::floatEq(q[BR].u, 1.0f));
    };

    "quad builder: indices form two triangles over the corners"_test = [] {
        expect(gfx::QuadIndices == ((std::array<uint16_t, 6>{ 0, 1, 2, 0, 2, 3 })));
    };
}
