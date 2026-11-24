#include <boost/ut.hpp>

#include "Renderer/Font/Font.hpp"
#include "Renderer/RendererFactory/RendererFactory.hpp"
#include "Renderer/SdlRenderer/SdlRenderer.hpp"
#include "Support/Headless.hpp"

#include <SDL3/SDL.h>

#include <array>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <memory>
#include <string_view>
#include <vector>

using namespace boost::ut;

using test::Blue;
using test::Green;
using test::Red;
using test::same;

namespace {

constexpr uint32_t Width = 16;
constexpr uint32_t Height = 8;

// A 16x8 offscreen SDL renderer on a software renderer, with pixel read-back.
struct Offscreen
{
    test::SoftwareRenderer sdl;
    std::unique_ptr<gfx::IOffscreenRenderer> renderer =
        gfx::makeOffscreenRenderer(gfx::Backend::Sdl3Cpu, sdl.renderer, Width, Height);

    gfx::Color pixel(int x, int y) const
    {
        return test::pixelAt(sdl.renderer, test::textureOf(*renderer), x, y);
    }

    // True when every pixel of the target has this colour.
    bool allPixels(gfx::Color color) const
    {
        for (int y = 0; y < static_cast<int>(Height); ++y)
            for (int x = 0; x < static_cast<int>(Width); ++x)
                if (!same(pixel(x, y), color))
                    return false;
        return true;
    }
};

std::vector<uint8_t> fontBytes()
{
    std::ifstream file(RENDER_TEST_ASSETS_DIR "/DejaVuSans.ttf", std::ios::binary);
    return std::vector<uint8_t>(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>{});
}

// RGBA8 pixels of one colour.
std::vector<uint8_t> filled(gfx::Color color, int count)
{
    std::vector<uint8_t> pixels;
    for (int i = 0; i < count; ++i)
        pixels.insert(pixels.end(), {color.r, color.g, color.b, color.a});
    return pixels;
}

// A 2x1 texture: Green on the left, Blue on the right.
gfx::TextureHandle greenBlue(gfx::IRenderer &renderer)
{
    const std::array<uint8_t, 8> pixels{0, 255, 0, 255, 0, 0, 255, 255};
    return renderer.createTexture(pixels.data(), 2, 1);
}

gfx::RectDraw screenRect(gfx::Rect dest, gfx::Color color, gfx::Layer layer = gfx::Layer::Game)
{
    return {dest, color, layer, gfx::Space::Screen};
}

gfx::SpriteDraw screenSprite(gfx::TextureHandle texture, gfx::Rect dest)
{
    gfx::SpriteDraw draw;
    draw.texture = texture;
    draw.dest = dest;
    draw.space = gfx::Space::Screen;
    return draw;
}

gfx::Vertex vertex(float x, float y, gfx::Color color)
{
    return {x, y, 0.0f, 0.0f, color.r, color.g, color.b, color.a};
}

} // namespace

void run_sdl_renderer_tests()
{
    "sdl renderer: a sprite shows its texture"_test = [] {
        Offscreen o;
        expect(o.renderer != nullptr) << fatal;
        const auto pixels = filled(Blue, 4);
        const gfx::TextureHandle texture = o.renderer->createTexture(pixels.data(), 2, 2);
        expect(texture != gfx::TextureHandle::Invalid) << fatal;
        o.renderer->beginFrame(Red);
        o.renderer->drawSprite(screenSprite(texture, {0.0f, 0.0f, 4.0f, 4.0f}));
        o.renderer->endFrame();
        expect(same(o.pixel(1, 1), Blue));
        expect(same(o.pixel(10, 6), Red));
    };

    "sdl renderer: flipX mirrors the source"_test = [] {
        Offscreen o;
        expect(o.renderer != nullptr) << fatal;
        gfx::SpriteDraw draw = screenSprite(greenBlue(*o.renderer), {0.0f, 0.0f, 4.0f, 2.0f});
        o.renderer->beginFrame(Red);
        o.renderer->drawSprite(draw);
        o.renderer->endFrame();
        expect(same(o.pixel(0, 0), Green));
        expect(same(o.pixel(3, 0), Blue));

        draw.flipX = true;
        o.renderer->beginFrame(Red);
        o.renderer->drawSprite(draw);
        o.renderer->endFrame();
        expect(same(o.pixel(0, 0), Blue));
        expect(same(o.pixel(3, 0), Green));
    };

    "sdl renderer: a source rect picks part of the texture"_test = [] {
        Offscreen o;
        expect(o.renderer != nullptr) << fatal;
        gfx::SpriteDraw draw = screenSprite(greenBlue(*o.renderer), {0.0f, 0.0f, 4.0f, 4.0f});
        draw.source = {1.0f, 0.0f, 1.0f, 1.0f};
        o.renderer->beginFrame(Red);
        o.renderer->drawSprite(draw);
        o.renderer->endFrame();
        expect(same(o.pixel(1, 1), Blue));
    };

    "sdl renderer: a rotated sprite still covers its centre"_test = [] {
        Offscreen o;
        expect(o.renderer != nullptr) << fatal;
        const auto pixels = filled(Blue, 4);
        gfx::SpriteDraw draw = screenSprite(o.renderer->createTexture(pixels.data(), 2, 2), {4.0f, 0.0f, 8.0f, 8.0f});
        draw.rotation = 0.785398f;
        o.renderer->beginFrame(Red);
        o.renderer->drawSprite(draw);
        o.renderer->endFrame();
        expect(same(o.pixel(8, 4), Blue));
        expect(same(o.pixel(4, 0), Red)) << "the corner leaves the rotated square";
    };

    "sdl renderer: line, triangle, quad, circle and polygon fill their shape"_test = [] {
        const auto drawn = [](auto draw, int x, int y) {
            Offscreen o;
            expect(o.renderer != nullptr) << fatal;
            o.renderer->beginFrame(Red);
            draw(*o.renderer);
            o.renderer->endFrame();
            return same(o.pixel(x, y), Blue);
        };
        expect(drawn([](gfx::IRenderer &r) { r.drawLine({{0, 4}, {16, 4}, 2.0f, Blue, gfx::Layer::Game, gfx::Space::Screen}); }, 8, 4))
            << "line";
        expect(drawn([](gfx::IRenderer &r) { r.drawTriangle({{{{0, 0}, {8, 0}, {0, 8}}}, Blue, gfx::Layer::Game, gfx::Space::Screen}); }, 1, 1))
            << "triangle";
        expect(drawn([](gfx::IRenderer &r) { r.drawQuad({{{{0, 0}, {4, 0}, {4, 4}, {0, 4}}}, Blue, gfx::Layer::Game, gfx::Space::Screen}); }, 2, 2))
            << "quad";
        expect(drawn([](gfx::IRenderer &r) { r.drawCircle({{8, 4}, 3.0f, Blue, 0, gfx::Layer::Game, gfx::Space::Screen}); }, 8, 4))
            << "circle";
        expect(drawn([](gfx::IRenderer &r) {
            const std::array<gfx::Vec2, 4> square{{{0, 0}, {6, 0}, {6, 6}, {0, 6}}};
            r.drawPolygon({square, Blue, gfx::Layer::Game, gfx::Space::Screen});
        }, 3, 3)) << "polygon";
    };

    "sdl renderer: a polygon under 3 points or that cannot be triangulated draws nothing"_test = [] {
        Offscreen o;
        expect(o.renderer != nullptr) << fatal;
        const std::array<gfx::Vec2, 2> two{{{0, 0}, {4, 4}}};
        const std::array<gfx::Vec2, 3> collinear{{{0, 0}, {4, 0}, {8, 0}}};
        o.renderer->beginFrame(Red);
        for (int i = 0; i < 2; ++i) {
            o.renderer->drawPolygon({two, Blue, gfx::Layer::Game, gfx::Space::Screen});
            o.renderer->drawPolygon({collinear, Blue, gfx::Layer::Game, gfx::Space::Screen});
        }
        o.renderer->endFrame();
        expect(o.allPixels(Red));
    };

    "sdl renderer: an untextured mesh is drawn with its vertex colours"_test = [] {
        Offscreen o;
        expect(o.renderer != nullptr) << fatal;
        const std::array<gfx::Vertex, 3> vertices{vertex(0, 0, Blue), vertex(8, 0, Blue), vertex(0, 8, Blue)};
        const std::array<uint16_t, 3> indices{0, 1, 2};
        o.renderer->beginFrame(Red);
        o.renderer->drawMesh({vertices, indices, gfx::TextureHandle::Invalid, gfx::Layer::Game, gfx::Space::Screen});
        o.renderer->endFrame();
        expect(same(o.pixel(1, 1), Blue));
    };

    "sdl renderer: a textured mesh samples its texture"_test = [] {
        Offscreen o;
        expect(o.renderer != nullptr) << fatal;
        const auto pixels = filled(Blue, 4);
        const gfx::TextureHandle texture = o.renderer->createTexture(pixels.data(), 2, 2);
        std::array<gfx::Vertex, 3> vertices{vertex(0, 0, gfx::White), vertex(8, 0, gfx::White), vertex(0, 8, gfx::White)};
        vertices[1].u = 1.0f;
        vertices[2].v = 1.0f;
        const std::array<uint16_t, 3> indices{0, 1, 2};
        o.renderer->beginFrame(Red);
        o.renderer->drawMesh({vertices, indices, texture, gfx::Layer::Game, gfx::Space::Screen});
        o.renderer->endFrame();
        expect(same(o.pixel(1, 1), Blue));
    };

    "sdl renderer: a malformed mesh or one with an unknown texture draws nothing"_test = [] {
        Offscreen o;
        expect(o.renderer != nullptr) << fatal;
        const std::array<gfx::Vertex, 3> vertices{vertex(0, 0, Blue), vertex(8, 0, Blue), vertex(0, 8, Blue)};
        const std::array<uint16_t, 3> outOfRange{0, 1, 5};
        const std::array<uint16_t, 3> indices{0, 1, 2};
        o.renderer->beginFrame(Red);
        for (int i = 0; i < 2; ++i) {
            o.renderer->drawMesh({vertices, outOfRange, gfx::TextureHandle::Invalid, gfx::Layer::Game, gfx::Space::Screen});
            o.renderer->drawMesh({vertices, indices, gfx::TextureHandle{999}, gfx::Layer::Game, gfx::Space::Screen});
        }
        o.renderer->endFrame();
        expect(o.allPixels(Red));
    };

    "sdl renderer: an empty mesh draws nothing"_test = [] {
        Offscreen o;
        expect(o.renderer != nullptr) << fatal;
        o.renderer->beginFrame(Red);
        o.renderer->drawMesh({});
        o.renderer->endFrame();
        expect(o.allPixels(Red));
    };

    "sdl renderer: text with a real font colours pixels; a null font draws nothing"_test = [] {
        Offscreen o;
        expect(o.renderer != nullptr) << fatal;
        const auto bytes = fontBytes();
        expect(!bytes.empty()) << fatal;
        const auto font = gfx::Font::create(*o.renderer, bytes, 8.0f);
        expect(font != nullptr) << fatal;

        gfx::TextDraw text;
        text.font = font.get();
        text.text = "H";
        text.color = Blue;
        text.space = gfx::Space::Screen;
        o.renderer->beginFrame(Red);
        o.renderer->drawText(text);
        o.renderer->endFrame();
        expect(!o.allPixels(Red));

        text.font = nullptr;
        o.renderer->beginFrame(Red);
        o.renderer->drawText(text);
        o.renderer->drawText(text);
        o.renderer->endFrame();
        expect(o.allPixels(Red));
    };

    "sdl renderer: layers draw Background, Game, Foreground whatever the call order"_test = [] {
        Offscreen o;
        expect(o.renderer != nullptr) << fatal;
        o.renderer->beginFrame(Red);
        o.renderer->drawRect(screenRect({0, 0, 4, 4}, Green, gfx::Layer::Foreground));
        o.renderer->drawRect(screenRect({0, 0, 4, 4}, Blue, gfx::Layer::Background));
        o.renderer->endFrame();
        expect(same(o.pixel(1, 1), Green));
    };

    "sdl renderer: world space follows the camera"_test = [] {
        Offscreen o;
        expect(o.renderer != nullptr) << fatal;
        const gfx::RectDraw world{{0, 0, 4, 4}, Blue, gfx::Layer::Game, gfx::Space::World};
        o.renderer->setCamera({{8, 4}, 1.0f});
        o.renderer->beginFrame(Red);
        o.renderer->drawRect(world);
        o.renderer->endFrame();
        expect(same(o.pixel(1, 1), Blue));

        o.renderer->setCamera({{12, 4}, 1.0f});
        o.renderer->beginFrame(Red);
        o.renderer->drawRect(world);
        o.renderer->endFrame();
        expect(same(o.pixel(1, 1), Red));
    };

    "sdl renderer: createTexture refuses empty pixels"_test = [] {
        Offscreen o;
        expect(o.renderer != nullptr) << fatal;
        const auto pixels = filled(Blue, 4);
        expect(o.renderer->createTexture(nullptr, 2, 2) == gfx::TextureHandle::Invalid);
        expect(o.renderer->createTexture(pixels.data(), 0, 2) == gfx::TextureHandle::Invalid);
        expect(o.renderer->createTexture(pixels.data(), 2, 0) == gfx::TextureHandle::Invalid);
    };

    "sdl renderer: textureSize knows its textures only"_test = [] {
        Offscreen o;
        expect(o.renderer != nullptr) << fatal;
        const auto pixels = filled(Blue, 6);
        const gfx::TextureHandle texture = o.renderer->createTexture(pixels.data(), 3, 2);
        const gfx::Vec2 size = o.renderer->textureSize(texture);
        expect(size.x == 3.0_f && size.y == 2.0_f);
        const gfx::Vec2 unknown = o.renderer->textureSize(gfx::TextureHandle{999});
        expect(unknown.x == 0.0_f && unknown.y == 0.0_f);
        o.renderer->destroyTexture(texture);
        const gfx::Vec2 destroyed = o.renderer->textureSize(texture);
        expect(destroyed.x == 0.0_f && destroyed.y == 0.0_f);
    };

    "sdl renderer: a texture destroyed during the frame still draws in that frame"_test = [] {
        Offscreen o;
        expect(o.renderer != nullptr) << fatal;
        const auto pixels = filled(Blue, 4);
        const gfx::TextureHandle texture = o.renderer->createTexture(pixels.data(), 2, 2);
        o.renderer->beginFrame(Red);
        o.renderer->drawSprite(screenSprite(texture, {0, 0, 4, 4}));
        o.renderer->destroyTexture(texture);
        o.renderer->endFrame();
        expect(same(o.pixel(1, 1), Blue));
        const gfx::Vec2 size = o.renderer->textureSize(texture);
        expect(size.x == 0.0_f && size.y == 0.0_f);
    };

    "sdl renderer: destroying an unknown handle is harmless"_test = [] {
        Offscreen o;
        expect(o.renderer != nullptr) << fatal;
        o.renderer->destroyTexture(gfx::TextureHandle{999});
        o.renderer->destroyTexture(gfx::TextureHandle{999});
        o.renderer->beginFrame(Red);
        o.renderer->endFrame();
        expect(same(o.pixel(0, 0), Red));
    };

    "sdl renderer: beginFrame twice and endFrame without beginFrame are ignored"_test = [] {
        Offscreen o;
        expect(o.renderer != nullptr) << fatal;
        o.renderer->beginFrame(Red);
        o.renderer->beginFrame(Blue);
        o.renderer->endFrame();
        expect(same(o.pixel(0, 0), Red));
        o.renderer->endFrame();
        expect(same(o.pixel(0, 0), Red));
    };

    "sdl renderer: draws outside a frame are ignored"_test = [] {
        Offscreen o;
        expect(o.renderer != nullptr) << fatal;
        o.renderer->drawRect(screenRect({0, 0, 4, 4}, Blue));
        o.renderer->drawRect(screenRect({0, 0, 4, 4}, Blue));
        o.renderer->beginFrame(Red);
        o.renderer->endFrame();
        expect(same(o.pixel(1, 1), Red));
    };

    "sdl renderer: a sprite with an invalid handle draws nothing"_test = [] {
        Offscreen o;
        expect(o.renderer != nullptr) << fatal;
        o.renderer->beginFrame(Red);
        o.renderer->drawSprite(screenSprite(gfx::TextureHandle{999}, {0, 0, 4, 4}));
        o.renderer->drawSprite(screenSprite(gfx::TextureHandle::Invalid, {0, 0, 4, 4}));
        o.renderer->endFrame();
        expect(same(o.pixel(1, 1), Red));
    };

    "sdl window: a frame with every draw call runs, twice"_test = [] {
        test::DummyWindow window;
        expect(window.window != nullptr) << SDL_GetError() << fatal;
        auto renderer = gfx::SdlRenderer::create(window.window, 64, 64);
        expect(renderer != nullptr) << SDL_GetError() << fatal;
        expect(renderer->outputTexture() == 0u) << "window mode presents; it has no output texture";

        const auto bytes = fontBytes();
        const auto font = gfx::Font::create(*renderer, bytes, 8.0f);
        expect(font != nullptr) << fatal;
        const auto pixels = filled(Blue, 4);
        const gfx::TextureHandle texture = renderer->createTexture(pixels.data(), 2, 2);
        const std::array<gfx::Vec2, 4> square{{{0, 0}, {6, 0}, {6, 6}, {0, 6}}};
        const std::array<gfx::Vertex, 3> vertices{vertex(0, 0, Blue), vertex(8, 0, Blue), vertex(0, 8, Blue)};
        const std::array<uint16_t, 3> indices{0, 1, 2};
        gfx::TextDraw text;
        text.font = font.get();
        text.text = "Hi";

        for (int frame = 0; frame < 2; ++frame) {
            renderer->setCamera({{32, 32}, 1.0f});
            renderer->beginFrame(Red);
            renderer->drawRect({{0, 0, 8, 8}, Blue});
            renderer->drawSprite(screenSprite(texture, {8, 8, 8, 8}));
            renderer->drawLine({{0, 0}, {64, 64}, 2.0f, Green});
            renderer->drawTriangle({{{{0, 0}, {8, 0}, {0, 8}}}, Green});
            renderer->drawQuad({{{{0, 0}, {4, 0}, {4, 4}, {0, 4}}}, Green});
            renderer->drawCircle({{32, 32}, 6.0f, Green});
            renderer->drawPolygon({square, Green});
            renderer->drawMesh({vertices, indices});
            renderer->drawText(text);
            renderer->endFrame();
        }
        renderer->destroyTexture(texture);
        expect(std::string_view(renderer->backendName()).starts_with("SDL3 / "));
    };

    // Smoke test: window mode presents, so where the resized frame lands cannot be read back here;
    // the offscreen resize tests check the sizes.
    "sdl window: a frame still runs after a resize, and after a zero-size resize"_test = [] {
        test::DummyWindow window;
        expect(window.window != nullptr) << SDL_GetError() << fatal;
        auto renderer = gfx::SdlRenderer::create(window.window, 64, 64);
        expect(renderer != nullptr) << SDL_GetError() << fatal;
        renderer->resize(32, 32);
        renderer->resize(0, 32);
        renderer->beginFrame(Red);
        renderer->drawRect({{0, 0, 8, 8}, Blue, gfx::Layer::Game, gfx::Space::Screen});
        renderer->endFrame();
        expect(std::string_view(renderer->backendName()).size() > 0_u);
    };

    "sdl renderer: offscreen refuses a zero size"_test = [] {
        test::SoftwareRenderer sdl;
        expect(gfx::makeOffscreenRenderer(gfx::Backend::Sdl3Cpu, sdl.renderer, 0, 8) == nullptr);
    };
}
