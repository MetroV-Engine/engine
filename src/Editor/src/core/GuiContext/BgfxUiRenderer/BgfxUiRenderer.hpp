/*
    Created on: 17/11/2026
    Filename: BgfxUiRenderer.hpp
    Description: ImGui drawn by bgfx on the editor's window; owns the process's BgfxContext.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#pragma once

#if ENGINE_RENDERER_BGFX

#include "core/GuiContext/UiRenderer.hpp"

#include <memory>
#include <string>

// Adapted from bgfx's examples/common/imgui (OcornutImguiContext): its render loop, textures and shaders,
// without its ImGui context, fonts or input, which GuiContext, ThemeManager and the SDL3 backend own.
class BgfxUiRenderer final : public UiRenderer
{
public:
    BgfxUiRenderer();
    ~BgfxUiRenderer() override;

    void attach(SDL_Window *window) override;
    void newFrame() override;
    void render(const ImVec4 &clear) override;

    bool setVSync(bool enabled) override;
    bool isVSync() const override;
    const char *name() const override;

    gfx::BgfxContext *bgfxContext() const override { return _context.get(); }

private:
    struct Device; // bgfx handles, kept out of this header

    void updateTextures(ImDrawData *drawData);
    void destroyTextures();

    SDL_Window *_window = nullptr;
    std::unique_ptr<gfx::BgfxContext> _context;
    std::unique_ptr<Device> _device;
    std::string _name;
    bool _imguiSdlBackend = false;
};

#endif
