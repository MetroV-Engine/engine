/*
    Created on: 9/22/26
    Filename: RendererFactory.hpp
    Description: Creates a renderer for a backend; the one line a caller changes to switch.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#ifndef RENDERER_WRAPPER_RENDERERFACTORY_HPP
#define RENDERER_WRAPPER_RENDERERFACTORY_HPP

#include "Renderer/IRenderer.hpp"

#include <memory>

struct SDL_Window;

namespace gfx {

    enum class Backend : uint8_t { Bgfx, Sdl3Cpu, Null };

    // Returns nullptr on failure, after logging the reason. Backend::Null ignores the window.
    std::unique_ptr<IRenderer> makeRenderer(Backend backend, SDL_Window* window,
                                            uint32_t width, uint32_t height);

}

#endif //RENDERER_WRAPPER_RENDERERFACTORY_HPP
