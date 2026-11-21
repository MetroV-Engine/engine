#include <boost/ut.hpp>

#include "Renderer/RendererFactory/RendererFactory.hpp"
#include "Support/Headless.hpp"

#include <string_view>

using namespace boost::ut;

void run_renderer_factory_tests()
{
    "renderer factory: Null ignores the window and names itself"_test = [] {
        auto renderer = gfx::makeRenderer(gfx::Backend::Null, nullptr, 32, 16);
        expect(renderer != nullptr) << fatal;
        expect(std::string_view(renderer->backendName()).size() > 0_u);
    };

    "renderer factory: SDL on a dummy window starts"_test = [] {
        test::DummyWindow window;
        expect(window.window != nullptr) << SDL_GetError() << fatal;
        auto renderer = gfx::makeRenderer(gfx::Backend::Sdl3Cpu, window.window, 64, 64);
        expect(renderer != nullptr) << SDL_GetError() << fatal;
        expect(std::string_view(renderer->backendName()).starts_with("SDL3 / "));
    };

    "renderer factory: SDL refuses a null window"_test = [] {
        expect(gfx::makeRenderer(gfx::Backend::Sdl3Cpu, nullptr, 64, 64) == nullptr);
    };

    "renderer factory: isBackendBuilt matches the build"_test = [] {
        expect(gfx::isBackendBuilt(gfx::Backend::Sdl3Cpu));
        expect(gfx::isBackendBuilt(gfx::Backend::Null));
#if ENGINE_RENDERER_BGFX
        expect(gfx::isBackendBuilt(gfx::Backend::Bgfx));
#else
        expect(!gfx::isBackendBuilt(gfx::Backend::Bgfx));
        expect(gfx::makeRenderer(gfx::Backend::Bgfx, nullptr, 64, 64) == nullptr);
#endif
    };

    "renderer factory: offscreen refuses every backend but SDL"_test = [] {
        test::SoftwareRenderer sdl;
        expect(gfx::makeOffscreenRenderer(gfx::Backend::Null, sdl.renderer, 8, 8) == nullptr);
    };
}
