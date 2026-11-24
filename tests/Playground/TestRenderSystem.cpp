#include <boost/ut.hpp>

#include "Components/CircleShape.hpp"
#include "Components/GradientBackground.hpp"
#include "Components/HudText.hpp"
#include "Components/PolygonShape.hpp"
#include "Components/RectShape.hpp"
#include "Components/Sprite.hpp"
#include "Components/Transform.hpp"
#include "Rendering/RenderSystem/RenderSystem.hpp"
#include "Rendering/TextureCache/TextureCache.hpp"

#include "Renderer/Font/Font.hpp"
#include "Renderer/NullRenderer/NullRenderer.hpp"

#include "ECS/ECS.hpp"

#include <SDL3/SDL.h>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

using namespace boost::ut;
using Kind = gfx::NullRenderer::Recorded::Kind;

namespace {

// A directory holding one 1024x512 sheet, written with SDL so the test needs no asset file.
struct SheetDirectory
{
    std::filesystem::path path = std::filesystem::temp_directory_path() / "playground-render-tests";

    SheetDirectory()
    {
        std::filesystem::create_directories(path);
        SDL_Surface *sheet = SDL_CreateSurface(1024, 512, SDL_PIXELFORMAT_RGBA32);
        SDL_SavePNG(sheet, (path / "sheet.png").string().c_str());
        SDL_DestroySurface(sheet);
    }
    ~SheetDirectory() { std::filesystem::remove_all(path); }
    std::string dir() const { return path.string() + "/"; }
};

std::unique_ptr<gfx::Font> loadFont(gfx::IRenderer &renderer)
{
    std::ifstream file(PLAYGROUND_TEST_FONT_PATH, std::ios::binary);
    const std::vector<uint8_t> ttf((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return gfx::Font::create(renderer, ttf, 24.0f);
}

std::vector<Kind> kinds(const gfx::NullRenderer &renderer)
{
    std::vector<Kind> result;
    for (const auto &recorded : renderer.recorded())
        result.push_back(recorded.kind);
    return result;
}

// A bool, so Boost.ut does not try to print the kinds on failure.
bool kindsAre(const gfx::NullRenderer &renderer, const std::vector<Kind> &expected)
{
    return kinds(renderer) == expected;
}

constexpr gfx::Vec2 Viewport{1280.0f, 720.0f};

} // namespace

void run_render_system_tests()
{
    "render: a RectShape becomes one world-space rect with its colour converted"_test = [] {
        ECS::Registry world;
        const ECS::Entity ground = world.spawnEntity("Ground");
        world.addComponent(ground, Transform{Vec2f{-2000.0f, 600.0f}, 0.0f});
        world.addComponent(ground, RectShape{Vec2f{6000.0f, 120.0f}, Rgba{1.0f, 0.0f, 0.0f, 1.0f}});
        gfx::NullRenderer renderer(1280, 720);
        TextureCache textures(renderer, "");
        renderer.beginFrame(gfx::Black);
        RenderSystem::draw(world, renderer, textures, nullptr, Viewport);
        renderer.endFrame();
        expect(fatal(renderer.recorded().size() == 1u));
        const gfx::RectDraw &rect = renderer.recorded()[0].rect;
        expect(rect.dest.x == -2000.0_f && rect.dest.y == 600.0_f && rect.dest.w == 6000.0_f && rect.dest.h == 120.0_f);
        expect(rect.color.r == 255_u && rect.color.g == 0_u && rect.color.a == 255_u);
        expect(rect.space == gfx::Space::World);
    };

    "render: topEdge adds a line 3 thick, lighter by 0.35"_test = [] {
        ECS::Registry world;
        const ECS::Entity platform = world.spawnEntity("Platform 1");
        world.addComponent(platform, Transform{Vec2f{10.0f, 20.0f}, 0.0f});
        RectShape shape{Vec2f{180.0f, 28.0f}, Rgba{0.0f, 0.0f, 0.0f, 1.0f}};
        shape.topEdge = true;
        world.addComponent(platform, shape);
        gfx::NullRenderer renderer(1280, 720);
        TextureCache textures(renderer, "");
        renderer.beginFrame(gfx::Black);
        RenderSystem::draw(world, renderer, textures, nullptr, Viewport);
        renderer.endFrame();
        expect(fatal(kindsAre(renderer, std::vector<Kind>{Kind::Rect, Kind::Line})));
        const gfx::LineDraw &line = renderer.recorded()[1].line;
        expect(line.thickness == 3.0_f);
        expect(line.from.x == 10.0_f && line.to.x == 190.0_f && line.from.y == 21.5_f);
        expect(line.color.r == 89_u) << "black lerped 0.35 toward white";
    };

    "render: a Sprite resolves its sheet and source frame"_test = [] {
        SheetDirectory sheets;
        ECS::Registry world;
        const ECS::Entity bull = world.spawnEntity("Bull");
        world.addComponent(bull, Transform{Vec2f{120.0f, 600.0f}, 0.0f});
        Sprite sprite;
        sprite.sheet = "sheet.png";
        sprite.frame = 1;
        sprite.flipX = true;
        world.addComponent(bull, sprite);
        gfx::NullRenderer renderer(1280, 720);
        TextureCache textures(renderer, sheets.dir());
        renderer.beginFrame(gfx::Black);
        RenderSystem::draw(world, renderer, textures, nullptr, Viewport);
        renderer.endFrame();
        expect(fatal(renderer.recorded().size() == 1u));
        const gfx::SpriteDraw &draw = renderer.recorded()[0].sprite;
        expect(draw.texture != gfx::TextureHandle::Invalid);
        expect(draw.source.x == 512.0_f && draw.source.y == 0.0_f && draw.source.w == 512.0_f && draw.source.h == 512.0_f);
        expect(draw.dest.x == 56.0_f && draw.dest.w == 128.0_f && draw.dest.h == 128.0_f);
        expect(draw.dest.y == 485.0_f) << "hooves 460/512 down the frame sit on the Transform";
        expect(draw.flipX);
    };

    "render: unknown sheet draws nothing and does not throw"_test = [] {
        ECS::Registry world;
        const ECS::Entity bull = world.spawnEntity("Bull");
        world.addComponent(bull, Transform{});
        Sprite sprite;
        sprite.sheet = "missing.png";
        world.addComponent(bull, sprite);
        gfx::NullRenderer renderer(1280, 720);
        TextureCache textures(renderer, "/nonexistent/");
        renderer.beginFrame(gfx::Black);
        expect(nothrow([&] {
            RenderSystem::draw(world, renderer, textures, nullptr, Viewport);
            RenderSystem::draw(world, renderer, textures, nullptr, Viewport);
        }));
        renderer.endFrame();
        expect(renderer.recorded().empty());
    };

    "render: draw order is sky, hills, ground, bull, bushes, HUD bar, HUD text"_test = [] {
        SheetDirectory sheets;
        ECS::Registry world;
        // Spawned out of order on purpose: the layers decide, not the spawn order.
        const ECS::Entity text = world.spawnEntity("HUD text");
        world.addComponent(text, Transform{Vec2f{40.0f, 4.0f}, 0.0f});
        world.addComponent(text, HudText{});
        const ECS::Entity bar = world.spawnEntity("HUD bar");
        world.addComponent(bar, Transform{});
        RectShape barShape{Vec2f{4000.0f, 36.0f}, Rgba{}, DrawLayer::Foreground};
        barShape.screenSpace = true;
        world.addComponent(bar, barShape);
        const ECS::Entity bush = world.spawnEntity("Bush 1");
        world.addComponent(bush, Transform{});
        world.addComponent(bush, CircleShape{40.0f, Rgba{}, DrawLayer::Foreground});
        const ECS::Entity bull = world.spawnEntity("Bull");
        world.addComponent(bull, Transform{});
        Sprite sprite;
        sprite.sheet = "sheet.png";
        world.addComponent(bull, sprite);
        const ECS::Entity ground = world.spawnEntity("Ground");
        world.addComponent(ground, Transform{});
        world.addComponent(ground, RectShape{});
        const ECS::Entity hills = world.spawnEntity("Far hills");
        world.addComponent(hills, Transform{});
        world.addComponent(hills, PolygonShape{{Vec2f{0, 0}, Vec2f{100, 0}, Vec2f{50, -80}}, Rgba{}});
        const ECS::Entity sky = world.spawnEntity("Sky");
        world.addComponent(sky, Transform{});
        world.addComponent(sky, GradientBackground{});

        gfx::NullRenderer renderer(1280, 720);
        const auto font = loadFont(renderer);
        expect(fatal(font != nullptr));
        TextureCache textures(renderer, sheets.dir());
        renderer.beginFrame(gfx::Black);
        RenderSystem::draw(world, renderer, textures, font.get(), Viewport);
        renderer.endFrame();
        expect(kindsAre(renderer, std::vector<Kind>{Kind::Mesh, Kind::Polygon, Kind::Rect, Kind::Sprite, Kind::Circle, Kind::Rect, Kind::Text}));
        expect(renderer.recorded().back().textContent == std::string(renderer.backendName()));
    };

    "render: no font, no HUD text, HUD bar still drawn"_test = [] {
        ECS::Registry world;
        const ECS::Entity text = world.spawnEntity("HUD text");
        world.addComponent(text, Transform{});
        world.addComponent(text, HudText{});
        const ECS::Entity bar = world.spawnEntity("HUD bar");
        world.addComponent(bar, Transform{});
        world.addComponent(bar, RectShape{Vec2f{4000.0f, 36.0f}, Rgba{}, DrawLayer::Foreground});
        gfx::NullRenderer renderer(1280, 720);
        TextureCache textures(renderer, "");
        renderer.beginFrame(gfx::Black);
        RenderSystem::draw(world, renderer, textures, nullptr, Viewport);
        renderer.endFrame();
        expect(kindsAre(renderer, std::vector<Kind>{Kind::Rect}));
    };

    "render: a viewport-anchored circle is placed by fraction of the viewport"_test = [] {
        ECS::Registry world;
        const ECS::Entity sun = world.spawnEntity("Sun");
        world.addComponent(sun, Transform{Vec2f{0.5f, 0.25f}, 0.0f});
        CircleShape shape{56.0f, Rgba{}, DrawLayer::Background};
        shape.screenSpace = true;
        shape.viewportAnchor = true;
        world.addComponent(sun, shape);
        gfx::NullRenderer renderer(1280, 720);
        TextureCache textures(renderer, "");
        renderer.beginFrame(gfx::Black);
        RenderSystem::draw(world, renderer, textures, nullptr, Viewport);
        renderer.endFrame();
        expect(fatal(renderer.recorded().size() == 1u));
        const gfx::CircleDraw &circle = renderer.recorded()[0].circle;
        expect(circle.center.x == 640.0_f && circle.center.y == 180.0_f);
        expect(circle.space == gfx::Space::Screen);
    };
}
