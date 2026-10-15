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

#include <SDL3/SDL.h>

class GuiContext
{
public:
    GuiContext(const char *title, int width, int height);
    ~GuiContext();

    GuiContext(const GuiContext &) = delete;
    GuiContext &operator=(const GuiContext &) = delete;

    bool isRunning() const { return _running; }
    void pollEvents();
    void beginFrame();
    void endFrame();

private:
    SDL_Window *_window = nullptr;
    SDL_Renderer *_renderer = nullptr;
    bool _imguiContext = false;
    bool _imguiSdlBackend = false;
    bool _imguiRendererBackend = false;
    bool _running = true;

    void destroy();
};
