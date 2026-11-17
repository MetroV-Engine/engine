/*
    Created on: 11/17/26
    Filename: ViewportView.cpp
    Description: The editor's Viewport for the game: input in, the rendered frame out as an image.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "ViewportView.hpp"

#include "Components/Camera.hpp"
#include "Components/Transform.hpp"
#include "Input/InputMapping/InputMapping.hpp"
#include "Input/InputState.hpp"
#include "Rendering/RenderSystem/RenderSystem.hpp"

#include "Renderer/Colors/Colors.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace {

constexpr gfx::Color Background = gfx::fromHex(0x181A20FF);
constexpr float PanSpeed = 700.0f; // world units per second at zoom 1
constexpr float ZoomStep = 1.12f;
constexpr float MinZoom = 0.2f;
constexpr float MaxZoom = 8.0f;

bool held(const ImGuiKey a, const ImGuiKey b)
{
    return ImGui::IsKeyDown(a) || ImGui::IsKeyDown(b);
}

} // namespace

ViewportView::ViewportView(ECS::Registry &world, gfx::IOffscreenRenderer &renderer, TextureCache &textures,
    const gfx::Font *font, std::function<PlayState()> playState)
    : _world(world), _renderer(renderer), _textures(textures), _font(font), _playState(std::move(playState))
{
    // Where the playground's play mode starts: 240 above the bull's hooves.
    _viewCamera.center = gfx::Vec2{120.0f, 360.0f};
}

void ViewportView::operator()(ImDrawList *drawList, const ImVec2 min, const ImVec2 max)
{
    const long width = std::lround(max.x - min.x);
    const long height = std::lround(max.y - min.y);
    if (width < 1 || height < 1)
        return;
    if (static_cast<uint32_t>(width) != _width || static_cast<uint32_t>(height) != _height) {
        _width = static_cast<uint32_t>(width);
        _height = static_cast<uint32_t>(height);
        _renderer.resize(_width, _height);
    }

    const PlayState state = _playState();
    // Play pressed on the toolbar focuses the toolbar: the game would not hear the keys until a click here.
    if (state == PlayState::Playing && _lastState != PlayState::Playing)
        ImGui::SetWindowFocus();
    _lastState = state;
    feedInput(state);
    if (state == PlayState::Stopped)
        navigate(ImGui::GetIO().DeltaTime);

    _renderer.beginFrame(Background);
    _renderer.setCamera(state == PlayState::Stopped ? _viewCamera : gameCamera());
    RenderSystem::draw(_world, _renderer, _textures, _font, gfx::Vec2{static_cast<float>(_width), static_cast<float>(_height)});
    _renderer.endFrame();

    drawList->AddImage(static_cast<ImTextureID>(_renderer.outputTexture()), min, max);
}

void ViewportView::feedInput(const PlayState state) const
{
    if (!_world.hasResource<InputState>())
        return;
    // Typing in another panel must never move the game.
    const bool active = state == PlayState::Playing && ImGui::IsWindowFocused();
    _world.getResource<InputState>() = inputFromKeys(active, held(ImGuiKey_A, ImGuiKey_LeftArrow),
        held(ImGuiKey_D, ImGuiKey_RightArrow), ImGui::IsKeyPressed(ImGuiKey_Space, false));
}

void ViewportView::navigate(const float deltaSeconds)
{
    if (!ImGui::IsWindowHovered())
        return;
    const float step = PanSpeed * deltaSeconds / _viewCamera.zoom; // constant speed on screen
    if (held(ImGuiKey_A, ImGuiKey_LeftArrow))
        _viewCamera.center.x -= step;
    if (held(ImGuiKey_D, ImGuiKey_RightArrow))
        _viewCamera.center.x += step;
    if (held(ImGuiKey_W, ImGuiKey_UpArrow))
        _viewCamera.center.y -= step;
    if (held(ImGuiKey_S, ImGuiKey_DownArrow))
        _viewCamera.center.y += step;

    const float wheel = ImGui::GetIO().MouseWheel;
    if (wheel != 0.0f)
        _viewCamera.zoom = std::clamp(_viewCamera.zoom * (wheel > 0.0f ? ZoomStep : 1.0f / ZoomStep), MinZoom, MaxZoom);
}

gfx::Camera2D ViewportView::gameCamera() const
{
    for (auto [entity, transform, camera] : _world.view<Transform, Camera>())
        return gfx::Camera2D{gfx::Vec2{transform.position.x, transform.position.y}, camera.zoom};
    return _viewCamera;
}
