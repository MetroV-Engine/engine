#include <boost/ut.hpp>

#include "Renderer/BasePath/BasePath.hpp"
#include "Renderer/RendererFactory/RendererFactory.hpp"
#include "Support/Headless.hpp"

#include <SDL3/SDL.h>

#include <cstdint>
#include <memory>
#include <string>

using namespace boost::ut;

using test::Blue;
using test::pixelAt;
using test::Red;
using test::same;
using test::SoftwareRenderer;
using test::textureOf;

void run_offscreen_renderer_tests()
{
    "offscreen: clear colour lands in the output texture"_test = [] {
        SoftwareRenderer sdl;
        expect(sdl.renderer != nullptr) << fatal;
        auto offscreen = gfx::makeOffscreenRenderer(gfx::Backend::Sdl3Cpu, sdl.renderer, 16, 8);
        expect(offscreen != nullptr) << fatal;
        offscreen->beginFrame(Red);
        offscreen->endFrame();
        expect(offscreen->outputTexture() != 0u);
        expect(same(pixelAt(sdl.renderer, textureOf(*offscreen), 0, 0), Red));
    };

    "offscreen: a screen-space rect is drawn into the texture"_test = [] {
        SoftwareRenderer sdl;
        auto offscreen = gfx::makeOffscreenRenderer(gfx::Backend::Sdl3Cpu, sdl.renderer, 16, 8);
        expect(offscreen != nullptr) << fatal;
        offscreen->beginFrame(Red);
        offscreen->drawRect({gfx::Rect{0.0f, 0.0f, 4.0f, 4.0f}, Blue, gfx::Layer::Game, gfx::Space::Screen});
        offscreen->endFrame();
        expect(same(pixelAt(sdl.renderer, textureOf(*offscreen), 1, 1), Blue));
        expect(same(pixelAt(sdl.renderer, textureOf(*offscreen), 10, 6), Red));
    };

    "offscreen: the previous render target and blend mode come back"_test = [] {
        SoftwareRenderer sdl;
        SDL_Texture *other = SDL_CreateTexture(sdl.renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, 4, 4);
        expect(other != nullptr) << fatal;
        SDL_SetRenderTarget(sdl.renderer, other);
        SDL_SetRenderDrawBlendMode(sdl.renderer, SDL_BLENDMODE_NONE);
        {
            auto offscreen = gfx::makeOffscreenRenderer(gfx::Backend::Sdl3Cpu, sdl.renderer, 16, 8);
            expect(offscreen != nullptr) << fatal;
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
        expect(offscreen != nullptr) << fatal;
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
        expect(offscreen != nullptr) << fatal;
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
