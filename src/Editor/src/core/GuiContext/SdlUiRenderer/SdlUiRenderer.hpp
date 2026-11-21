/*
    Created on: 17/11/2026
    Filename: SdlUiRenderer.hpp
    Description: ImGui drawn by an SDL_Renderer on the editor's window.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#pragma once

#include "core/GuiContext/UiRenderer.hpp"

class SdlUiRenderer final : public UiRenderer
{
public:
    ~SdlUiRenderer() override;

    void attach(SDL_Window *window) override;
    void newFrame() override;
    void render(const ImVec4 &clear) override;

    bool setVSync(bool enabled) override;
    bool isVSync() const override;
    const char *name() const override;

    SDL_Renderer *sdlRenderer() const override { return _renderer; }

private:
    SDL_Renderer *_renderer = nullptr;
    bool _imguiSdlBackend = false;
    bool _imguiRendererBackend = false;
};
