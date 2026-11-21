// SDL without a display or a GPU: a software renderer on a plain surface, and a window on the
// "dummy" video driver. Shared by the renderer tests.
#pragma once

#include "Renderer/IOffscreenRenderer.hpp"
#include "Renderer/RendererTypes.hpp"

#include <SDL3/SDL.h>

namespace test {

// A software renderer on a plain surface: no window, no video driver needed.
struct SoftwareRenderer
{
    SDL_Surface *surface = SDL_CreateSurface(64, 64, SDL_PIXELFORMAT_RGBA32);
    SDL_Renderer *renderer = surface != nullptr ? SDL_CreateSoftwareRenderer(surface) : nullptr;

    SoftwareRenderer() = default;
    SoftwareRenderer(const SoftwareRenderer &) = delete;
    SoftwareRenderer &operator=(const SoftwareRenderer &) = delete;

    ~SoftwareRenderer()
    {
        if (renderer != nullptr)
            SDL_DestroyRenderer(renderer);
        if (surface != nullptr)
            SDL_DestroySurface(surface);
    }
};

// A hidden window on SDL's "dummy" video driver, whose renderers are software ones. SDL video is
// started for this window and stopped with it, so tests stay independent of each other.
struct DummyWindow
{
    SDL_Window *window = nullptr;

    explicit DummyWindow(int width = 64, int height = 64)
    {
        SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
        SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
        if (!SDL_InitSubSystem(SDL_INIT_VIDEO))
            return;
        m_video = true;
        window = SDL_CreateWindow("tests_render", width, height, SDL_WINDOW_HIDDEN);
    }

    DummyWindow(const DummyWindow &) = delete;
    DummyWindow &operator=(const DummyWindow &) = delete;

    ~DummyWindow()
    {
        if (window != nullptr)
            SDL_DestroyWindow(window);
        if (m_video)
            SDL_QuitSubSystem(SDL_INIT_VIDEO);
    }

  private:
    bool m_video = false;
};

inline SDL_Texture *textureOf(const gfx::IOffscreenRenderer &offscreen)
{
    return reinterpret_cast<SDL_Texture *>(offscreen.outputTexture());
}

// Reads one pixel of a target texture back, leaving the renderer's target as it was.
inline gfx::Color pixelAt(SDL_Renderer *renderer, SDL_Texture *texture, int x, int y)
{
    SDL_Texture *previous = SDL_GetRenderTarget(renderer);
    SDL_SetRenderTarget(renderer, texture);
    const SDL_Rect area{x, y, 1, 1};
    SDL_Surface *read = SDL_RenderReadPixels(renderer, &area);
    SDL_SetRenderTarget(renderer, previous);
    gfx::Color color{0, 0, 0, 0};
    if (read != nullptr) {
        SDL_ReadSurfacePixel(read, 0, 0, &color.r, &color.g, &color.b, &color.a);
        SDL_DestroySurface(read);
    }
    return color;
}

inline bool same(gfx::Color a, gfx::Color b)
{
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}

inline constexpr gfx::Color Red{255, 0, 0, 255};
inline constexpr gfx::Color Green{0, 255, 0, 255};
inline constexpr gfx::Color Blue{0, 0, 255, 255};

} // namespace test
