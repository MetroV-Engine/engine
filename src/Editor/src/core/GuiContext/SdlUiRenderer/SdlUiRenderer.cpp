/*
    Created on: 17/11/2026
    Filename: SdlUiRenderer.cpp
    Description: ImGui drawn by an SDL_Renderer on the editor's window.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "SdlUiRenderer.hpp"
#include "core/Logger/Logger.hpp"

#include <SDL3/SDL.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

#include <stdexcept>
#include <string>

SdlUiRenderer::~SdlUiRenderer()
{
    if (_imguiRendererBackend)
        ImGui_ImplSDLRenderer3_Shutdown();
    if (_imguiSdlBackend)
        ImGui_ImplSDL3_Shutdown();
    if (_renderer)
        SDL_DestroyRenderer(_renderer);
}

void SdlUiRenderer::attach(SDL_Window *window)
{
    _renderer = SDL_CreateRenderer(window, nullptr);
    if (!_renderer)
        throw std::runtime_error(std::string("SDL_CreateRenderer failed: ") + SDL_GetError());

    if (!ImGui_ImplSDL3_InitForSDLRenderer(window, _renderer))
        throw std::runtime_error("ImGui SDL3 backend init failed");
    _imguiSdlBackend = true;

    if (!ImGui_ImplSDLRenderer3_Init(_renderer))
        throw std::runtime_error("ImGui SDLRenderer3 backend init failed");
    _imguiRendererBackend = true;
}

void SdlUiRenderer::newFrame()
{
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
}

void SdlUiRenderer::render(const ImVec4 &clear)
{
    const ImGuiIO &io = ImGui::GetIO();
    SDL_SetRenderScale(_renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
    SDL_SetRenderDrawColorFloat(_renderer, clear.x, clear.y, clear.z, 1.0f);
    SDL_RenderClear(_renderer);
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), _renderer);
    SDL_RenderPresent(_renderer);
}

bool SdlUiRenderer::setVSync(bool enabled)
{
    if (SDL_SetRenderVSync(_renderer, enabled ? 1 : SDL_RENDERER_VSYNC_DISABLED))
        return true;
    Logger::instance().error("Renderer", std::string("SDL_SetRenderVSync failed: ") + SDL_GetError());
    return false;
}

bool SdlUiRenderer::isVSync() const
{
    int vsync = 0;
    return SDL_GetRenderVSync(_renderer, &vsync) && vsync != 0;
}

const char *SdlUiRenderer::name() const
{
    const char *name = SDL_GetRendererName(_renderer);
    return name ? name : "unknown";
}
