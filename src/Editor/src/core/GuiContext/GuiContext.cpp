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
#include "core/Logger/Logger.hpp"

#include "core/GuiContext/SdlUiRenderer/SdlUiRenderer.hpp"
#if ENGINE_RENDERER_BGFX
#include "core/GuiContext/BgfxUiRenderer/BgfxUiRenderer.hpp"
#endif

#include <imgui.h>
#include <imgui_impl_sdl3.h>

#include <algorithm>
#include <stdexcept>
#include <string>

namespace {

// Sleep granularity while minimized: keeps CPU near zero, still reacts quickly.
constexpr int IdleWaitMs = 100;
// Frames longer than this are reported: the UI would feel unresponsive.
constexpr double SlowFrameSeconds = 0.1;

std::unique_ptr<UiRenderer> makeUiRenderer(UiBackend backend)
{
    switch (backend) {
    case UiBackend::Sdl:
        return std::make_unique<SdlUiRenderer>();
    case UiBackend::Bgfx:
#if ENGINE_RENDERER_BGFX
        return std::make_unique<BgfxUiRenderer>();
#else
        throw std::runtime_error("the bgfx UI backend is not built (ENGINE_RENDERER_BGFX=OFF)");
#endif
    }
    throw std::runtime_error("unknown UI backend");
}

} // namespace

GuiContext::GuiContext(const char *title, int width, int height, UiBackend backend)
{
    try {
        // Closing the window only sends SDL_EVENT_WINDOW_CLOSE_REQUESTED, so the
        // editor can ask for confirmation first (see setCloseRequestHandler).
        // SDL_EVENT_QUIT (Ctrl+C, system shutdown) still quits immediately.
        SDL_SetHint(SDL_HINT_QUIT_ON_LAST_WINDOW_CLOSE, "0");
        if (!SDL_Init(SDL_INIT_VIDEO))
            throw std::runtime_error(std::string("SDL_Init failed: ") + SDL_GetError());

        _window = SDL_CreateWindow(title, width, height, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
        if (!_window)
            throw std::runtime_error(std::string("SDL_CreateWindow failed: ") + SDL_GetError());

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        _imguiContext = true;
        // Applies the default theme at the window's display scale.
        applyDisplayScale();

        _ui = makeUiRenderer(backend);
        _ui->attach(_window);

        _events.subscribeAll([](const SDL_Event &event) { ImGui_ImplSDL3_ProcessEvent(&event); });
        _events.subscribe(SDL_EVENT_QUIT, [this](const SDL_Event &) { _running = false; });
        _events.subscribe(SDL_EVENT_WINDOW_CLOSE_REQUESTED, [this](const SDL_Event &) {
            if (_closeRequestHandler)
                _closeRequestHandler();
            else
                _running = false;
        });
        // Nothing to draw while minimized/hidden. Any sign that the window is
        // visible again leaves idle mode: some compositors (e.g. Wayland) do
        // not always report the restore itself, but do report focus/exposure.
        for (const SDL_EventType type : {SDL_EVENT_WINDOW_MINIMIZED, SDL_EVENT_WINDOW_HIDDEN})
            _events.subscribe(type, [this](const SDL_Event &) { _idle = true; });
        for (const SDL_EventType type : {SDL_EVENT_WINDOW_RESTORED, SDL_EVENT_WINDOW_MAXIMIZED,
                 SDL_EVENT_WINDOW_SHOWN, SDL_EVENT_WINDOW_EXPOSED, SDL_EVENT_WINDOW_FOCUS_GAINED})
            _events.subscribe(type, [this](const SDL_Event &) { _idle = false; });
        // Keep the normal geometry up to date, to save it (see getWindowState).
        rememberNormalGeometry();
        for (const SDL_EventType type : {SDL_EVENT_WINDOW_RESIZED, SDL_EVENT_WINDOW_MOVED})
            _events.subscribe(type, [this](const SDL_Event &) { rememberNormalGeometry(); });

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
    // The UI renderer shuts its ImGui backends down: before the context and the window it uses.
    _ui.reset();
    if (_imguiContext)
        ImGui::DestroyContext();
    if (_window)
        SDL_DestroyWindow(_window);
    SDL_Quit();

    _imguiContext = false;
    _window = nullptr;
}

void GuiContext::setFullscreen(bool fullscreen)
{
    if (!SDL_SetWindowFullscreen(_window, fullscreen))
        Logger::instance().error("Window", std::string("SDL_SetWindowFullscreen failed: ") + SDL_GetError());
}

bool GuiContext::isFullscreen() const
{
    return (SDL_GetWindowFlags(_window) & SDL_WINDOW_FULLSCREEN) != 0;
}

void GuiContext::rememberNormalGeometry()
{
    if (SDL_GetWindowFlags(_window) & (SDL_WINDOW_MAXIMIZED | SDL_WINDOW_FULLSCREEN | SDL_WINDOW_MINIMIZED))
        return;
    SDL_GetWindowSize(_window, &_normalWidth, &_normalHeight);
    SDL_GetWindowPosition(_window, &_normalX, &_normalY);
}

WindowState GuiContext::getWindowState() const
{
    const SDL_WindowFlags flags = SDL_GetWindowFlags(_window);
    WindowState state;
    state.width = _normalWidth;
    state.height = _normalHeight;
    state.hasPosition = SDL_strcmp(SDL_GetCurrentVideoDriver(), "wayland") != 0;
    state.x = _normalX;
    state.y = _normalY;
    state.maximized = (flags & SDL_WINDOW_MAXIMIZED) != 0;
    state.fullscreen = (flags & SDL_WINDOW_FULLSCREEN) != 0;
    return state;
}

void GuiContext::applyWindowState(const WindowState &state)
{
    constexpr int MinWidth = 640;
    constexpr int MinHeight = 400;
    SDL_Rect usable{0, 0, 0, 0};
    const bool hasBounds = SDL_GetDisplayUsableBounds(SDL_GetDisplayForWindow(_window), &usable);

    if (state.fullscreen)
        setFullscreen(false);
    SDL_RestoreWindow(_window);
    if (state.width > 0 && state.height > 0) {
        int width = std::max(state.width, MinWidth);
        int height = std::max(state.height, MinHeight);
        if (hasBounds) {
            width = std::min(width, usable.w);
            height = std::min(height, usable.h);
        }
        SDL_SetWindowSize(_window, width, height);
        _normalWidth = width;
        _normalHeight = height;
    }
    // Only if the saved position is still on a display.
    if (state.hasPosition) {
        const SDL_Point corner{state.x, state.y};
        if (SDL_GetDisplayForPoint(&corner) != 0)
            SDL_SetWindowPosition(_window, state.x, state.y);
    }
    if (state.maximized)
        SDL_MaximizeWindow(_window);
    if (state.fullscreen)
        setFullscreen(true);
}

void GuiContext::setWindowSize(int width, int height)
{
    // While fullscreen, SDL keeps the size and applies it when leaving fullscreen.
    if (!SDL_SetWindowSize(_window, width, height))
        Logger::instance().error("Window", std::string("SDL_SetWindowSize failed: ") + SDL_GetError());
    SDL_SetWindowPosition(_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
}

bool GuiContext::isVSync() const
{
    return _ui->isVSync();
}

const char *GuiContext::getRendererName() const
{
    return _ui->name();
}

void GuiContext::setVSync(bool enabled)
{
    _ui->setVSync(enabled);
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
    char text[32];
    SDL_snprintf(text, sizeof(text), "%.2f", scale);
    Logger::instance().info("Window", std::string("UI scale set to ") + text);
}

void GuiContext::beginFrame()
{
    _timer.tick();
    if (_timer.getDeltaTime() > SlowFrameSeconds) {
        char text[48];
        SDL_snprintf(text, sizeof(text), "Slow frame: %.1f ms", _timer.getDeltaTime() * 1000.0);
        Logger::instance().warning("Performance", text);
    }

    _ui->newFrame();
    ImGui::NewFrame();
}

void GuiContext::present()
{
    ImGui::Render();
    _ui->render(_themes.current().colors.background);
}
