/*
    Created on: 17/11/2026
    Filename: UiRenderer.hpp
    Description: What draws the editor's ImGui frames into the window: SDL_Renderer or bgfx.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#pragma once

#include <imgui.h>

#include <cstdint>

struct SDL_Renderer;
struct SDL_Window;

namespace gfx {
class BgfxContext;
}

// Chosen once, when the GuiContext is created.
enum class UiBackend : std::uint8_t
{
    Sdl,
    Bgfx, // Needs ENGINE_RENDERER_BGFX
};

// The only part of the editor that draws: SDL3 keeps the window, events, input
// and time in both cases. Owns the ImGui platform and renderer backends.
class UiRenderer
{
public:
    virtual ~UiRenderer() = default;

    // Starts both ImGui backends on the window; throws std::runtime_error on failure.
    virtual void attach(SDL_Window *window) = 0;
    virtual void newFrame() = 0;
    // Draws ImGui::GetDrawData() over the clear colour, then shows the frame.
    virtual void render(const ImVec4 &clear) = 0;

    virtual bool setVSync(bool enabled) = 0;
    virtual bool isVSync() const = 0;
    virtual const char *name() const = 0;

    // The SDL_Renderer the UI draws with, or the bgfx context: null on the other backend.
    virtual SDL_Renderer *sdlRenderer() const { return nullptr; }
    virtual gfx::BgfxContext *bgfxContext() const { return nullptr; }
};
