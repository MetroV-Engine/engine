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

#if ENGINE_RENDERER_BGFX
#include "Renderer/BgfxRenderer/BgfxRenderer.hpp"
#endif

#include <iostream>

namespace gfx {

    bool isBackendBuilt(const Backend backend)
    {
#if ENGINE_RENDERER_BGFX
        (void)backend;
        return true;
#else
        return backend != Backend::Bgfx;
#endif
    }

    std::unique_ptr<IRenderer> makeRenderer(const Backend backend, SDL_Window* window,
                                            const uint32_t width, const uint32_t height)
    {
        switch (backend) {
            case Backend::Sdl3Cpu:
                return SdlRenderer::create(window, width, height);

            case Backend::Null:
                return std::make_unique<NullRenderer>(width, height);

            case Backend::Bgfx:
#if ENGINE_RENDERER_BGFX
                return BgfxRenderer::create(window, width, height);
#else
                std::cerr << "[RendererFactory] ERROR: the bgfx backend is not built (ENGINE_RENDERER_BGFX=OFF)\n";
                return nullptr;
#endif
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

    std::unique_ptr<IOffscreenRenderer> makeOffscreenRenderer(BgfxContext& context, const uint32_t width,
                                                              const uint32_t height)
    {
#if ENGINE_RENDERER_BGFX
        return BgfxRenderer::createOffscreen(context, width, height);
#else
        (void)context;
        (void)width;
        (void)height;
        std::cerr << "[RendererFactory] ERROR: the bgfx backend is not built (ENGINE_RENDERER_BGFX=OFF)\n";
        return nullptr;
#endif
    }

}
