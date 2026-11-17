/*
    Created on: 11/17/26
    Filename: Level.cpp
    Description: Spawns the playground level: sky, sun, hills, ground, platforms, bushes, bull, camera, HUD.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "Level.hpp"

#include "Components/Animator.hpp"
#include "Components/Camera.hpp"
#include "Components/CircleShape.hpp"
#include "Components/GradientBackground.hpp"
#include "Components/HudText.hpp"
#include "Components/PlayerController.hpp"
#include "Components/PolygonShape.hpp"
#include "Components/RectShape.hpp"
#include "Components/Sprite.hpp"
#include "Components/Transform.hpp"
#include "Input/InputState.hpp"
#include "Rendering/Convert.hpp"

#include "Renderer/Colors/Colors.hpp"

#include <array>
#include <cmath>
#include <string>
#include <tuple>
#include <vector>

namespace {

constexpr float GroundTop = 600.0f;
constexpr float LevelLeft = -2000.0f;
constexpr int HillSegments = 40;
constexpr float HillStep = 150.0f;
constexpr float HillWavelength = 0.0021f; // radians per world unit
constexpr float HillBelowGround = 20.0f;  // the ground rect hides the hills' flat base

constexpr std::array<float, 4> BushPositions{-420.0f, 300.0f, 860.0f, 1500.0f};
constexpr float BushSink = 6.0f; // circle centres sit just below the ground line

struct BushLobe
{
    float offset;
    float radius;
};
constexpr std::array<BushLobe, 3> BushLobes{{{-44.0f, 38.0f}, {0.0f, 52.0f}, {44.0f, 38.0f}}};

Rgba hex(const uint32_t rgba)
{
    return toRgba(gfx::fromHex(rgba));
}

std::vector<Vec2f> hillOutline(const float minHeight, const float swell, const float phase)
{
    std::vector<Vec2f> outline;
    outline.reserve(HillSegments + 3);
    for (int i = 0; i <= HillSegments; ++i) {
        const float x = LevelLeft + static_cast<float>(i) * HillStep;
        const float rise = 0.5f + 0.5f * std::sin(x * HillWavelength + phase);
        outline.push_back(Vec2f{x, GroundTop - minHeight - swell * rise});
    }
    const float right = LevelLeft + static_cast<float>(HillSegments) * HillStep;
    outline.push_back(Vec2f{right, GroundTop + HillBelowGround});
    outline.push_back(Vec2f{LevelLeft, GroundTop + HillBelowGround});
    return outline;
}

ECS::Entity spawnAt(ECS::Registry &world, const std::string &name, const Vec2f position)
{
    const ECS::Entity entity = world.spawnEntity(name);
    world.addComponent(entity, Transform{position, 0.0f});
    return entity;
}

void spawnSky(ECS::Registry &world)
{
    const ECS::Entity sky = spawnAt(world, "Sky", Vec2f{});
    world.addComponent(sky, GradientBackground{hex(0x1B2A49FF), hex(0xE08A5CFF)});

    // Screen space at a fraction of the viewport: something this far away does not move with the camera.
    const Rgba sun = toRgba(gfx::fromHsv(gfx::Hsv{40.0f, 0.65f, 1.0f}));
    Rgba glow = sun;
    glow.a = 70.0f / 255.0f;
    for (const auto &[name, radius, color] : {std::tuple{"Sun glow", 96.0f, glow}, std::tuple{"Sun", 56.0f, sun}}) {
        const ECS::Entity entity = spawnAt(world, name, Vec2f{0.72f, 0.32f});
        CircleShape shape{radius, color, DrawLayer::Background};
        shape.screenSpace = true;
        shape.viewportAnchor = true;
        world.addComponent(entity, shape);
    }

    const ECS::Entity far = spawnAt(world, "Far hills", Vec2f{});
    world.addComponent(far, PolygonShape{hillOutline(150.0f, 170.0f, 0.0f), hex(0x4A3F5CFF), DrawLayer::Background});
    const ECS::Entity near = spawnAt(world, "Near hills", Vec2f{});
    world.addComponent(near, PolygonShape{hillOutline(50.0f, 90.0f, 2.4f), hex(0x2E3345FF), DrawLayer::Background});
}

void spawnGround(ECS::Registry &world)
{
    RectShape ground{Vec2f{6000.0f, 120.0f}, hex(0x3C483AFF), DrawLayer::Game};
    ground.topEdge = true;
    world.addComponent(spawnAt(world, "Ground", Vec2f{LevelLeft, GroundTop}), ground);

    for (int i = 0; i < 8; ++i) {
        RectShape platform{Vec2f{180.0f, 28.0f}, hex(0x5C6880FF), DrawLayer::Game};
        platform.topEdge = true;
        const Vec2f position{static_cast<float>(-800 + i * 320), static_cast<float>(420 - (i % 4) * 90)};
        world.addComponent(spawnAt(world, "Platform " + std::to_string(i + 1), position), platform);
    }
}

void spawnBushes(ECS::Registry &world)
{
    for (std::size_t bush = 0; bush < BushPositions.size(); ++bush) {
        for (std::size_t lobe = 0; lobe < BushLobes.size(); ++lobe) {
            const std::string name = "Bush " + std::to_string(bush + 1) + "." + std::to_string(lobe + 1);
            const Vec2f center{BushPositions[bush] + BushLobes[lobe].offset, GroundTop + BushSink};
            world.addComponent(spawnAt(world, name, center), CircleShape{BushLobes[lobe].radius, hex(0x1F3327FF), DrawLayer::Foreground});
        }
    }
}

void spawnBull(ECS::Registry &world)
{
    const ECS::Entity bull = spawnAt(world, "Bull", Vec2f{120.0f, GroundTop});
    Sprite sprite;
    sprite.sheet = "bongo_repos_spritesheet.png";
    world.addComponent(bull, sprite);
    Animator animator;
    animator.walk = AnimationClip{"bongo_marche_spritesheet.png", 8, 12.0f};
    animator.idle = AnimationClip{"bongo_repos_spritesheet.png", 12, 6.0f};
    animator.jump = AnimationClip{"bongo_saut_spritesheet.png", 3, 0.0f};
    world.addComponent(bull, animator);
    world.addComponent(bull, PlayerController{});

    // Centred 240 above the hooves, as the playground's play mode starts.
    world.addComponent(spawnAt(world, "Camera", Vec2f{120.0f, GroundTop - 240.0f}), Camera{1.0f, "Bull"});
}

void spawnHud(ECS::Registry &world)
{
    // Wider than any viewport: screen space, so it never scrolls.
    RectShape bar{Vec2f{8192.0f, 36.0f}, toRgba(gfx::withAlpha(gfx::White, 40)), DrawLayer::Foreground};
    bar.screenSpace = true;
    world.addComponent(spawnAt(world, "HUD bar", Vec2f{}), bar);
    world.addComponent(spawnAt(world, "HUD text", Vec2f{40.0f, 4.0f}), HudText{});
}

} // namespace

void spawnLevel(ECS::Registry &world)
{
    spawnSky(world);
    spawnGround(world);
    spawnBushes(world);
    spawnBull(world);
    spawnHud(world);
    if (!world.hasResource<InputState>())
        world.setResource<InputState>();
}
