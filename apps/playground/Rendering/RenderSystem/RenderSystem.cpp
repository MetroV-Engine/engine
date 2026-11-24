/*
    Created on: 11/17/26
    Filename: RenderSystem.cpp
    Description: Draws every visible entity of the world through IRenderer.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "RenderSystem.hpp"

#include "Components/CircleShape.hpp"
#include "Components/GradientBackground.hpp"
#include "Components/HudText.hpp"
#include "Components/PolygonShape.hpp"
#include "Components/RectShape.hpp"
#include "Components/Sprite.hpp"
#include "Components/Transform.hpp"
#include "Rendering/Convert.hpp"
#include "Rendering/TextureCache/TextureCache.hpp"

#include "Renderer/Colors/Colors.hpp"
#include "Renderer/IRenderer.hpp"

#include <array>
#include <vector>

namespace {

constexpr float EdgeThickness = 3.0f;
constexpr float EdgeLightness = 0.35f;
constexpr DrawLayer Layers[] = {DrawLayer::Background, DrawLayer::Game, DrawLayer::Foreground};

gfx::Vertex vertex(const float x, const float y, const gfx::Color color)
{
    return gfx::Vertex{x, y, 0.0f, 0.0f, color.r, color.g, color.b, color.a};
}

void drawGradients(ECS::Registry &world, gfx::IRenderer &renderer, const gfx::Vec2 viewport)
{
    for (auto [entity, gradient] : world.view<GradientBackground>()) {
        const gfx::Color top = toColor(gradient.top);
        const gfx::Color bottom = toColor(gradient.bottom);
        const std::array<gfx::Vertex, 4> quad{
            {vertex(0.0f, 0.0f, top), vertex(viewport.x, 0.0f, top), vertex(viewport.x, viewport.y, bottom), vertex(0.0f, viewport.y, bottom)}};
        const std::array<uint16_t, 6> indices{0, 1, 2, 0, 2, 3};
        renderer.drawMesh(gfx::MeshDraw{quad, indices, gfx::TextureHandle::Invalid, gfx::Layer::Background, gfx::Space::Screen});
    }
}

void drawCircles(ECS::Registry &world, gfx::IRenderer &renderer, const DrawLayer layer, const gfx::Vec2 viewport)
{
    for (auto [entity, transform, circle] : world.view<Transform, CircleShape>()) {
        if (circle.layer != layer)
            continue;
        gfx::Vec2 center = toVec2(transform.position);
        if (circle.screenSpace && circle.viewportAnchor)
            center = gfx::Vec2{center.x * viewport.x, center.y * viewport.y};
        renderer.drawCircle({center, circle.radius, toColor(circle.color), 0, toLayer(layer), toSpace(circle.screenSpace)});
    }
}

void drawPolygons(ECS::Registry &world, gfx::IRenderer &renderer, const DrawLayer layer)
{
    std::vector<gfx::Vec2> points;
    for (auto [entity, transform, polygon] : world.view<Transform, PolygonShape>()) {
        if (polygon.layer != layer)
            continue;
        points.clear();
        for (const Vec2f &point : polygon.points)
            points.push_back(gfx::Vec2{transform.position.x + point.x, transform.position.y + point.y});
        renderer.drawPolygon({points, toColor(polygon.color), toLayer(layer), gfx::Space::World});
    }
}

void drawRects(ECS::Registry &world, gfx::IRenderer &renderer, const DrawLayer layer)
{
    for (auto [entity, transform, rect] : world.view<Transform, RectShape>()) {
        if (rect.layer != layer)
            continue;
        const gfx::Rect dest{transform.position.x, transform.position.y, rect.size.x, rect.size.y};
        const gfx::Color color = toColor(rect.color);
        const gfx::Space space = toSpace(rect.screenSpace);
        renderer.drawRect({dest, color, toLayer(layer), space});
        if (rect.topEdge) {
            const float y = dest.y + EdgeThickness * 0.5f;
            renderer.drawLine({gfx::Vec2{dest.x, y}, gfx::Vec2{dest.x + dest.w, y}, EdgeThickness,
                gfx::lerp(color, gfx::White, EdgeLightness), toLayer(layer), space});
        }
    }
}

void drawSprites(ECS::Registry &world, gfx::IRenderer &renderer, TextureCache &textures, const DrawLayer layer)
{
    for (auto [entity, transform, sprite] : world.view<Transform, Sprite>()) {
        if (sprite.layer != layer)
            continue;
        const gfx::TextureHandle texture = textures.get(sprite.sheet);
        if (texture == gfx::TextureHandle::Invalid)
            continue;
        gfx::SpriteDraw draw;
        draw.texture = texture;
        draw.source = gfx::Rect{static_cast<float>(sprite.frame) * sprite.frameSize.x, 0.0f, sprite.frameSize.x, sprite.frameSize.y};
        draw.dest = gfx::Rect{transform.position.x - sprite.drawSize.x * sprite.anchor.x,
            transform.position.y - sprite.drawSize.y * sprite.anchor.y, sprite.drawSize.x, sprite.drawSize.y};
        draw.rotation = transform.rotation;
        draw.flipX = sprite.flipX;
        draw.tint = toColor(sprite.tint);
        draw.layer = toLayer(layer);
        renderer.drawSprite(draw);
    }
}

void drawHudTexts(ECS::Registry &world, gfx::IRenderer &renderer, const gfx::Font *font)
{
    if (font == nullptr)
        return;
    for (auto [entity, transform, hud] : world.view<Transform, HudText>()) {
        gfx::TextDraw draw;
        draw.font = font;
        draw.text = renderer.backendName();
        draw.position = toVec2(transform.position);
        draw.color = toColor(hud.color);
        draw.layer = gfx::Layer::Foreground;
        draw.space = gfx::Space::Screen;
        renderer.drawText(draw);
    }
}

} // namespace

namespace RenderSystem {

void draw(ECS::Registry &world, gfx::IRenderer &renderer, TextureCache &textures, const gfx::Font *font, const gfx::Vec2 viewport)
{
    // Within a layer submission order decides: circles under polygons under rects under sprites.
    drawGradients(world, renderer, viewport);
    for (const DrawLayer layer : Layers) {
        drawCircles(world, renderer, layer, viewport);
        drawPolygons(world, renderer, layer);
        drawRects(world, renderer, layer);
        drawSprites(world, renderer, textures, layer);
    }
    drawHudTexts(world, renderer, font);
}

}
