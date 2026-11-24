#include <boost/ut.hpp>

#if ENGINE_RENDERER_BGFX
#include "Renderer/BgfxContext/BgfxContext.hpp"
#include "Renderer/BgfxRenderer/BgfxDrawState.hpp"
#include "Renderer/RendererFactory/RendererFactory.hpp"

#include <memory>
#endif

using namespace boost::ut;

#if ENGINE_RENDERER_BGFX
namespace {

// What a blend factor (BGFX_STATE_BLEND_* >> BGFX_STATE_BLEND_SHIFT) weighs alpha by.
float alphaFactor(uint64_t factor, float srcA)
{
    switch (factor << BGFX_STATE_BLEND_SHIFT) {
    case BGFX_STATE_BLEND_ZERO: return 0.0f;
    case BGFX_STATE_BLEND_ONE: return 1.0f;
    case BGFX_STATE_BLEND_SRC_ALPHA: return srcA;
    case BGFX_STATE_BLEND_INV_SRC_ALPHA: return 1.0f - srcA;
    default: return -1.0f;
    }
}

// The alpha a draw with this state stores over a pixel of alpha dstA (blend equation ADD).
float storedAlpha(uint64_t state, float srcA, float dstA)
{
    const uint64_t blend = (state & BGFX_STATE_BLEND_MASK) >> BGFX_STATE_BLEND_SHIFT;
    const uint64_t srcAlphaFactor = (blend >> 8) & 0xF;
    const uint64_t dstAlphaFactor = (blend >> 12) & 0xF;
    return srcA * alphaFactor(srcAlphaFactor, srcA) + dstA * alphaFactor(dstAlphaFactor, srcA);
}

} // namespace
#endif

void run_bgfx_offscreen_tests()
{
#if ENGINE_RENDERER_BGFX
    using Device = gfx::BgfxContext::Device;

    "bgfx offscreen: a translucent draw keeps the target opaque, as SDL's blend does"_test = [] {
        expect(storedAlpha(gfx::bgfxDrawState::Sprite, 0.5f, 1.0f) == 1.0_f);
        expect(storedAlpha(gfx::bgfxDrawState::Sprite, 0.25f, 1.0f) == 1.0_f);
        expect(storedAlpha(gfx::bgfxDrawState::Sprite, 0.5f, 0.0f) == 0.5_f) << "over nothing, the draw's own alpha";
    };

    "bgfx offscreen: outputs a valid texture id"_test = [] {
        auto context = gfx::BgfxContext::create(nullptr, 64, 64, false, Device::Noop);
        expect(context != nullptr) << fatal;
        auto offscreen = gfx::makeOffscreenRenderer(*context, 32, 16);
        expect(offscreen != nullptr) << fatal;
        expect(offscreen->outputTexture() != 0u);
    };

    "bgfx offscreen: endFrame never advances the frame"_test = [] {
        auto context = gfx::BgfxContext::create(nullptr, 64, 64, false, Device::Noop);
        expect(context != nullptr) << fatal;
        auto offscreen = gfx::makeOffscreenRenderer(*context, 32, 16);
        expect(offscreen != nullptr) << fatal;
        const uint32_t before = context->frameNumber();
        offscreen->beginFrame(gfx::Black);
        offscreen->drawRect({gfx::Rect{0.0f, 0.0f, 4.0f, 4.0f}, gfx::White, gfx::Layer::Game, gfx::Space::Screen});
        offscreen->endFrame();
        expect(context->frameNumber() == before) << "the UI owns bgfx::frame()";
        context->frame();
    };

    "bgfx offscreen: resize changes the texture; zero is ignored"_test = [] {
        auto context = gfx::BgfxContext::create(nullptr, 64, 64, false, Device::Noop);
        expect(context != nullptr) << fatal;
        auto offscreen = gfx::makeOffscreenRenderer(*context, 32, 16);
        expect(offscreen != nullptr) << fatal;
        const std::uintptr_t first = offscreen->outputTexture();
        offscreen->resize(64, 32);
        const std::uintptr_t second = offscreen->outputTexture();
        expect(second != 0u && second != first) << "a new framebuffer, a new texture";
        offscreen->resize(0, 32);
        expect(offscreen->outputTexture() == second);
    };

    "bgfx offscreen: two renderers get two views"_test = [] {
        auto context = gfx::BgfxContext::create(nullptr, 64, 64, false, Device::Noop);
        expect(context != nullptr) << fatal;
        auto first = gfx::makeOffscreenRenderer(*context, 16, 16);
        auto second = gfx::makeOffscreenRenderer(*context, 16, 16);
        expect(first != nullptr && second != nullptr) << fatal;
        expect(first->outputTexture() != second->outputTexture());
        expect(context->allocateView() == 2_u) << "views 0 and 1 are taken";
    };

    "bgfx offscreen: destroying it leaves the context usable"_test = [] {
        auto context = gfx::BgfxContext::create(nullptr, 64, 64, false, Device::Noop);
        expect(context != nullptr) << fatal;
        {
            auto offscreen = gfx::makeOffscreenRenderer(*context, 16, 16);
            expect(offscreen != nullptr) << fatal;
        }
        expect(nothrow([&] { context->frame(); }));
    };

    "bgfx offscreen: names its backend"_test = [] {
        auto context = gfx::BgfxContext::create(nullptr, 64, 64, false, Device::Noop);
        expect(context != nullptr) << fatal;
        auto offscreen = gfx::makeOffscreenRenderer(*context, 16, 16);
        expect(offscreen != nullptr) << fatal;
        expect(std::string(offscreen->backendName()) == "bgfx / Noop");
    };
#endif
}
