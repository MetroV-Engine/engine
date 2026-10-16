/*
    Created on: 30/03/2026
    Filename: GuiContext.cpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "GuiContext.hpp"

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

#include <stdexcept>
#include <string>

GuiContext::GuiContext(const char *title, int width, int height)
{
    try {
        if (!SDL_Init(SDL_INIT_VIDEO))
            throw std::runtime_error(std::string("SDL_Init failed: ") + SDL_GetError());

        if (!SDL_CreateWindowAndRenderer(title, width, height, SDL_WINDOW_RESIZABLE, &_window, &_renderer))
            throw std::runtime_error(std::string("SDL_CreateWindowAndRenderer failed: ") + SDL_GetError());

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        _imguiContext = true;

        if (!ImGui_ImplSDL3_InitForSDLRenderer(_window, _renderer))
            throw std::runtime_error("ImGui SDL3 backend init failed");
        _imguiSdlBackend = true;

        if (!ImGui_ImplSDLRenderer3_Init(_renderer))
            throw std::runtime_error("ImGui SDLRenderer3 backend init failed");
        _imguiRendererBackend = true;

        _events.subscribeAll([](const SDL_Event &event) { ImGui_ImplSDL3_ProcessEvent(&event); });
        _events.subscribe(SDL_EVENT_QUIT, [this](const SDL_Event &) { _running = false; });
        _events.subscribe(SDL_EVENT_WINDOW_CLOSE_REQUESTED, [this](const SDL_Event &) { _running = false; });
    } catch (...) {
        destroy();
        throw;
    }
}

GuiContext::~GuiContext()
{
    destroy();
}

void GuiContext::destroy()
{
    if (_imguiRendererBackend)
        ImGui_ImplSDLRenderer3_Shutdown();
    if (_imguiSdlBackend)
        ImGui_ImplSDL3_Shutdown();
    if (_imguiContext)
        ImGui::DestroyContext();
    if (_renderer)
        SDL_DestroyRenderer(_renderer);
    if (_window)
        SDL_DestroyWindow(_window);
    SDL_Quit();

    _imguiRendererBackend = _imguiSdlBackend = _imguiContext = false;
    _renderer = nullptr;
    _window = nullptr;
}

void GuiContext::beginFrame()
{
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
}

void GuiContext::endFrame()
{
    ImGui::Render();
    SDL_SetRenderDrawColor(_renderer, 30, 30, 30, 255);
    SDL_RenderClear(_renderer);
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), _renderer);
    SDL_RenderPresent(_renderer);
}
