#include <boost/ut.hpp>

#include "Renderer/RendererFactory/RendererFactory.hpp"

#if ENGINE_RENDERER_BGFX
#include "Renderer/BgfxContext/BgfxContext.hpp"
#include "Renderer/BgfxContext/BgfxStartCheck.hpp"
#endif

using namespace boost::ut;

void run_bgfx_context_tests()
{
#if ENGINE_RENDERER_BGFX
    "bgfx context: Noop starts without a window and names itself"_test = [] {
        auto context = gfx::BgfxContext::create(nullptr, 64, 64, false, gfx::BgfxContext::Device::Noop);
        expect(context != nullptr) << fatal;
        expect(std::string(context->rendererName()) == "Noop");
    };

    "bgfx context: frame advances the frame number"_test = [] {
        auto context = gfx::BgfxContext::create(nullptr, 64, 64, false, gfx::BgfxContext::Device::Noop);
        expect(context != nullptr) << fatal;
        const uint32_t before = context->frameNumber();
        context->frame();
        context->frame();
        expect(context->frameNumber() == before + 2);
    };

    "bgfx context: views are allocated upward and never reach the UI view"_test = [] {
        auto context = gfx::BgfxContext::create(nullptr, 64, 64, false, gfx::BgfxContext::Device::Noop);
        expect(context != nullptr) << fatal;
        expect(context->allocateView() == 0_u);
        expect(context->allocateView() == 1_u);
        expect(gfx::BgfxContext::UiView == 255_u);
    };

    "bgfx context: only one at a time"_test = [] {
        auto first = gfx::BgfxContext::create(nullptr, 64, 64, false, gfx::BgfxContext::Device::Noop);
        expect(first != nullptr) << fatal;
        expect(gfx::BgfxContext::create(nullptr, 64, 64, false, gfx::BgfxContext::Device::Noop) == nullptr);
        first.reset();
        expect(gfx::BgfxContext::create(nullptr, 64, 64, false, gfx::BgfxContext::Device::Noop) != nullptr);
    };

    "bgfx context: reset keeps the backbuffer size and vsync"_test = [] {
        auto context = gfx::BgfxContext::create(nullptr, 64, 64, false, gfx::BgfxContext::Device::Noop);
        expect(context != nullptr) << fatal;
        context->reset(128, 32, true);
        expect(context->width() == 128_u && context->height() == 32_u);
        expect(context->vsync());
    };

    "bgfx context: falling back to Noop without asking for it is a failure"_test = [] {
        using gfx::bgfxStartFailure;
        expect(bgfxStartFailure(false, bgfx::RendererType::Noop) != nullptr) << "no GPU renderer started";
        expect(bgfxStartFailure(true, bgfx::RendererType::Noop) == nullptr) << "Noop was asked for";
        expect(bgfxStartFailure(false, bgfx::RendererType::Vulkan) == nullptr);
        expect(bgfxStartFailure(false, bgfx::RendererType::OpenGL) == nullptr);
    };

    "factory: bgfx is reported as built"_test = [] { expect(gfx::isBackendBuilt(gfx::Backend::Bgfx)); };
#else
    "factory: bgfx is reported as not built"_test = [] { expect(!gfx::isBackendBuilt(gfx::Backend::Bgfx)); };
#endif
    "factory: SDL and Null are always built"_test = [] {
        expect(gfx::isBackendBuilt(gfx::Backend::Sdl3Cpu) && gfx::isBackendBuilt(gfx::Backend::Null));
    };
}
