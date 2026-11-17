/*
    Created on: 11/17/26
    Filename: ViewportView.hpp
    Description: The editor's Viewport for the game: input in, the rendered frame out as an image.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#pragma once

#include "Rendering/TextureCache/TextureCache.hpp"

#include "Renderer/IOffscreenRenderer.hpp"

#include "ECS/ECS.hpp"
#include "scene/PlayController/PlayController.hpp"

#include <imgui.h>

#include <cstdint>
#include <functional>

namespace gfx {
class Font;
}

// Called by the Viewport each editor frame, inside its window: Stopped shows a free view camera
// (WASD/arrows pan, wheel zooms, while hovered); Playing and Paused show the game's Camera entity.
class ViewportView
{
public:
    // Everything passed in must outlive the view.
    ViewportView(ECS::Registry &world, gfx::IOffscreenRenderer &renderer, TextureCache &textures, const gfx::Font *font,
        std::function<PlayState()> playState);

    void operator()(ImDrawList *drawList, ImVec2 min, ImVec2 max);

private:
    void feedInput(PlayState state) const;
    void navigate(float deltaSeconds);
    gfx::Camera2D gameCamera() const;

    ECS::Registry &_world;
    gfx::IOffscreenRenderer &_renderer;
    TextureCache &_textures;
    const gfx::Font *_font;
    std::function<PlayState()> _playState;
    gfx::Camera2D _viewCamera;
    PlayState _lastState = PlayState::Stopped;
    uint32_t _width = 0;
    uint32_t _height = 0;
};
