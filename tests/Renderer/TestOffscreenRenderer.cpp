#include <boost/ut.hpp>

#include "Renderer/BasePath/BasePath.hpp"
#include "Renderer/RendererFactory/RendererFactory.hpp"

#include <SDL3/SDL.h>

#include <cstdint>
#include <memory>
#include <string>

using namespace boost::ut;

namespace {

// A software renderer on a plain surface: no window, no video driver needed.
struct SoftwareRenderer
{
    SDL_Surface *surface = SDL_CreateSurface(64, 64, SDL_PIXELFORMAT_RGBA32);
    SDL_Renderer *renderer = surface != nullptr ? SDL_CreateSoftwareRenderer(surface) : nullptr;

    ~SoftwareRenderer()
    {
        if (renderer != nullptr)
            SDL_DestroyRenderer(renderer);
        if (surface != nullptr)
            SDL_DestroySurface(surface);
    }
};

SDL_Texture *textureOf(const gfx::IOffscreenRenderer &offscreen)
{
    return reinterpret_cast<SDL_Texture *>(offscreen.outputTexture());
}

// Reads one pixel of the offscreen output back, leaving the renderer's target as it was.
gfx::Color pixelAt(SDL_Renderer *renderer, SDL_Texture *texture, int x, int y)
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

bool same(gfx::Color a, gfx::Color b)
{
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}

constexpr gfx::Color Red{255, 0, 0, 255};
constexpr gfx::Color Blue{0, 0, 255, 255};

} // namespace

void run_offscreen_renderer_tests()
{
    "offscreen: clear colour lands in the output texture"_test = [] {
        SoftwareRenderer sdl;
        expect(fatal(sdl.renderer != nullptr));
        auto offscreen = gfx::makeOffscreenRenderer(gfx::Backend::Sdl3Cpu, sdl.renderer, 16, 8);
        expect(fatal(offscreen != nullptr));
        offscreen->beginFrame(Red);
        offscreen->endFrame();
        expect(offscreen->outputTexture() != 0u);
        expect(same(pixelAt(sdl.renderer, textureOf(*offscreen), 0, 0), Red));
    };

    "offscreen: a screen-space rect is drawn into the texture"_test = [] {
        SoftwareRenderer sdl;
        auto offscreen = gfx::makeOffscreenRenderer(gfx::Backend::Sdl3Cpu, sdl.renderer, 16, 8);
        expect(fatal(offscreen != nullptr));
        offscreen->beginFrame(Red);
        offscreen->drawRect({gfx::Rect{0.0f, 0.0f, 4.0f, 4.0f}, Blue, gfx::Layer::Game, gfx::Space::Screen});
        offscreen->endFrame();
        expect(same(pixelAt(sdl.renderer, textureOf(*offscreen), 1, 1), Blue));
        expect(same(pixelAt(sdl.renderer, textureOf(*offscreen), 10, 6), Red));
    };

    "offscreen: the previous render target and blend mode come back"_test = [] {
        SoftwareRenderer sdl;
        SDL_Texture *other = SDL_CreateTexture(sdl.renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, 4, 4);
        expect(fatal(other != nullptr));
        SDL_SetRenderTarget(sdl.renderer, other);
        SDL_SetRenderDrawBlendMode(sdl.renderer, SDL_BLENDMODE_NONE);
        {
            auto offscreen = gfx::makeOffscreenRenderer(gfx::Backend::Sdl3Cpu, sdl.renderer, 16, 8);
            expect(fatal(offscreen != nullptr));
            offscreen->beginFrame(Red);
            offscreen->drawRect({gfx::Rect{0.0f, 0.0f, 4.0f, 4.0f}, Blue, gfx::Layer::Game, gfx::Space::Screen});
            offscreen->endFrame();
        }
        SDL_BlendMode mode = SDL_BLENDMODE_BLEND;
        SDL_GetRenderDrawBlendMode(sdl.renderer, &mode);
        expect(SDL_GetRenderTarget(sdl.renderer) == other);
        expect(mode == SDL_BLENDMODE_NONE);
        SDL_SetRenderTarget(sdl.renderer, nullptr);
        SDL_DestroyTexture(other);
    };

    "offscreen: resize recreates the texture; zero is ignored"_test = [] {
        SoftwareRenderer sdl;
        auto offscreen = gfx::makeOffscreenRenderer(gfx::Backend::Sdl3Cpu, sdl.renderer, 16, 8);
        expect(fatal(offscreen != nullptr));
        float w = 0.0f;
        float h = 0.0f;
        offscreen->resize(32, 16);
        SDL_GetTextureSize(textureOf(*offscreen), &w, &h);
        expect(w == 32.0_f && h == 16.0_f);
        offscreen->resize(0, 16);
        SDL_GetTextureSize(textureOf(*offscreen), &w, &h);
        expect(w == 32.0_f && h == 16.0_f);
    };

    "offscreen: destroying it leaves the borrowed renderer usable"_test = [] {
        SoftwareRenderer sdl;
        auto offscreen = gfx::makeOffscreenRenderer(gfx::Backend::Sdl3Cpu, sdl.renderer, 16, 8);
        expect(fatal(offscreen != nullptr));
        offscreen.reset();
        expect(SDL_RenderClear(sdl.renderer));
    };

    "offscreen: only SDL is supported; null renderer refused"_test = [] {
        SoftwareRenderer sdl;
        expect(gfx::makeOffscreenRenderer(gfx::Backend::Bgfx, sdl.renderer, 8, 8) == nullptr);
        expect(gfx::makeOffscreenRenderer(gfx::Backend::Sdl3Cpu, nullptr, 8, 8) == nullptr);
    };

    "base path: relative path is appended to the executable directory"_test = [] {
        const std::string path = tools::pathNextToExecutable("assets/x.png");
        expect(path.size() > std::string("assets/x.png").size());
        expect(path.ends_with("assets/x.png"));
    };
}
