/*
    Created on: 9/22/26
    Filename: RendererFactory.cpp
    Description: Creates a renderer for a backend; the one line a caller changes to switch.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "Renderer/RendererFactory/RendererFactory.hpp"

#include "Renderer/NullRenderer/NullRenderer.hpp"
#include "Renderer/SdlRenderer/SdlRenderer.hpp"

#include <iostream>

namespace gfx {

    std::unique_ptr<IRenderer> makeRenderer(const Backend backend, SDL_Window* window,
                                            const uint32_t width, const uint32_t height)
    {
        switch (backend) {
            case Backend::Sdl3Cpu:
                return SdlRenderer::create(window, width, height);

            case Backend::Null:
                return std::make_unique<NullRenderer>(width, height);

            case Backend::Bgfx:
                // Kept in the enum so callers don't change when the bgfx backend lands.
                std::cerr << "[RendererFactory] ERROR: the bgfx backend is not built yet; use Backend::Sdl3Cpu\n";
                return nullptr;
        }

        return nullptr;
    }

    std::unique_ptr<IOffscreenRenderer> makeOffscreenRenderer(const Backend backend, SDL_Renderer* borrowed,
                                                              const uint32_t width, const uint32_t height)
    {
        if (backend != Backend::Sdl3Cpu) {
            std::cerr << "[RendererFactory] ERROR: only Backend::Sdl3Cpu can render offscreen\n";
            return nullptr;
        }

        return SdlRenderer::createOffscreen(borrowed, width, height);
    }

}
