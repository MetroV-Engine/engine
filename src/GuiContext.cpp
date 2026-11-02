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

namespace {

// Sleep granularity while minimized: keeps CPU near zero, still reacts quickly.
constexpr int IdleWaitMs = 100;
// Frames longer than this are reported: the UI would feel unresponsive.
constexpr double SlowFrameSeconds = 0.1;

} // namespace

GuiContext::GuiContext(const char *title, int width, int height)
{
    try {
        if (!SDL_Init(SDL_INIT_VIDEO))
            throw std::runtime_error(std::string("SDL_Init failed: ") + SDL_GetError());

        if (!SDL_CreateWindowAndRenderer(title, width, height, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY, &_window, &_renderer))
            throw std::runtime_error(std::string("SDL_CreateWindowAndRenderer failed: ") + SDL_GetError());

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        _imguiContext = true;
        // Applies the default theme at the window's display scale.
        applyDisplayScale();

        if (!ImGui_ImplSDL3_InitForSDLRenderer(_window, _renderer))
            throw std::runtime_error("ImGui SDL3 backend init failed");
        _imguiSdlBackend = true;

        if (!ImGui_ImplSDLRenderer3_Init(_renderer))
            throw std::runtime_error("ImGui SDLRenderer3 backend init failed");
        _imguiRendererBackend = true;

        _events.subscribeAll([](const SDL_Event &event) { ImGui_ImplSDL3_ProcessEvent(&event); });
        _events.subscribe(SDL_EVENT_QUIT, [this](const SDL_Event &) { _running = false; });
        _events.subscribe(SDL_EVENT_WINDOW_CLOSE_REQUESTED, [this](const SDL_Event &) { _running = false; });
        // Nothing to draw while minimized/hidden. Any sign that the window is
        // visible again leaves idle mode: some compositors (e.g. Wayland) do
        // not always report the restore itself, but do report focus/exposure.
        for (const SDL_EventType type : {SDL_EVENT_WINDOW_MINIMIZED, SDL_EVENT_WINDOW_HIDDEN})
            _events.subscribe(type, [this](const SDL_Event &) { _idle = true; });
        for (const SDL_EventType type : {SDL_EVENT_WINDOW_RESTORED, SDL_EVENT_WINDOW_MAXIMIZED,
                 SDL_EVENT_WINDOW_SHOWN, SDL_EVENT_WINDOW_EXPOSED, SDL_EVENT_WINDOW_FOCUS_GAINED})
            _events.subscribe(type, [this](const SDL_Event &) { _idle = false; });
        // Moving to a monitor with another scale, or changing the system scale.
        _events.subscribe(SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED, [this](const SDL_Event &) { applyDisplayScale(); });
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

void GuiContext::setFullscreen(bool fullscreen)
{
    if (!SDL_SetWindowFullscreen(_window, fullscreen))
        SDL_Log("SDL_SetWindowFullscreen failed: %s", SDL_GetError());
}

bool GuiContext::isFullscreen() const
{
    return (SDL_GetWindowFlags(_window) & SDL_WINDOW_FULLSCREEN) != 0;
}

void GuiContext::setWindowSize(int width, int height)
{
    // While fullscreen, SDL keeps the size and applies it when leaving fullscreen.
    if (!SDL_SetWindowSize(_window, width, height))
        SDL_Log("SDL_SetWindowSize failed: %s", SDL_GetError());
    SDL_SetWindowPosition(_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
}

void GuiContext::setVSync(bool enabled)
{
    if (!SDL_SetRenderVSync(_renderer, enabled ? 1 : SDL_RENDERER_VSYNC_DISABLED))
        SDL_Log("SDL_SetRenderVSync failed: %s", SDL_GetError());
}

void GuiContext::run(const std::function<void()> &buildUi)
{
    while (_running) {
        if (_idle) {
            _events.waitEvents(IdleWaitMs);
            _timer.reset();
            continue;
        }

        processEvents();
        if (!_running)
            break;
        if (_idle)
            continue;
        beginFrame();
        buildUi();
        present();
    }
}


void GuiContext::applyDisplayScale()
{
    // Display scale = pixel density * content scale. Pixel density is already
    // handled by the framebuffer scale, so only the content scale is applied.
    const float density = SDL_GetWindowPixelDensity(_window);
    float scale = density > 0.0f ? SDL_GetWindowDisplayScale(_window) / density : 1.0f;
    if (scale <= 0.0f)
        scale = 1.0f;
    if (scale == _displayScale)
        return;

    _themes.setScale(scale);
    _displayScale = scale;
    SDL_Log("UI scale set to %.2f", scale);
}

void GuiContext::beginFrame()
{
    _timer.tick();
    if (_timer.getDeltaTime() > SlowFrameSeconds)
        SDL_Log("Slow frame: %.1f ms", _timer.getDeltaTime() * 1000.0);

    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
}

void GuiContext::present()
{
    ImGui::Render();
    const ImGuiIO &io = ImGui::GetIO();
    SDL_SetRenderScale(_renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
    const ImVec4 &clear = _themes.current().colors.background;
    SDL_SetRenderDrawColorFloat(_renderer, clear.x, clear.y, clear.z, 1.0f);
    SDL_RenderClear(_renderer);
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), _renderer);
    SDL_RenderPresent(_renderer);
}
