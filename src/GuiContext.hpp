/*
    Created on: 30/03/2026
    Filename: GuiContext.hpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#pragma once

#include "EventManager.hpp"
#include "FrameTimer.hpp"

#include <SDL3/SDL.h>
#include <imgui.h>

#include <functional>

class GuiContext
{
public:
    GuiContext(const char *title, int width, int height);
    ~GuiContext();

    GuiContext(const GuiContext &) = delete;
    GuiContext &operator=(const GuiContext &) = delete;

    // Frame lifecycle. run() loops over the four stages until the window is
    // closed; each stage is also public for custom loops:
    //   1. processEvents() - dispatch pending SDL events (input, resize, quit...)
    //   2. beginFrame()    - measure frame time, start a new ImGui frame
    //   3. buildUi()       - caller code: declare every widget of this frame
    //   4. present()       - render ImGui draw data and show it on screen
    // There is no fixed frame rate: VSync paces the loop when enabled. While
    // the window is minimized or hidden nothing is drawn and the loop sleeps
    // until an event arrives.
    void run(const std::function<void()> &buildUi);

    void processEvents() { _events.pollEvents(); }
    void beginFrame();
    void present();

    bool isRunning() const { return _running; }
    void stop() { _running = false; }
    EventManager &events() { return _events; }
    const FrameTimer &timer() const { return _timer; }
    float getDisplayScale() const { return _displayScale; }

    // Window settings, applied immediately.
    void setFullscreen(bool fullscreen);
    void setWindowSize(int width, int height);
    void setVSync(bool enabled);

private:
    SDL_Window *_window = nullptr;
    SDL_Renderer *_renderer = nullptr;
    bool _imguiContext = false;
    bool _imguiSdlBackend = false;
    bool _imguiRendererBackend = false;
    bool _running = true;
    bool _idle = false;
    EventManager _events;
    FrameTimer _timer;
    ImGuiStyle _baseStyle;
    float _displayScale = 0.0f;

    void destroy();
    // Rescales the ImGui style and fonts to the window's current display scale.
    void applyDisplayScale();
};
