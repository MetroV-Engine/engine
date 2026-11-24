/*
    Created on: 11/17/26
    Filename: BgfxContext.cpp
    Description: The process-wide bgfx: init on a window, reset, frame, views; one owner at a time.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "Renderer/BgfxContext/BgfxContext.hpp"
#include "Renderer/BgfxContext/BgfxStartCheck.hpp"

#include "Renderer/SdlWindowBridge/SdlWindowBridge.hpp"

#include <SDL3/SDL.h>
#include <bgfx/bgfx.h>

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

namespace gfx {

    namespace {
        uint32_t resetFlags(const bool vsync)
        {
            return vsync ? BGFX_RESET_VSYNC : BGFX_RESET_NONE;
        }

        // bgfx's default output, plus a count of its warnings (it prefixes each with "BGFX WARN ").
        class TraceCallback final : public bgfx::CallbackI {
            public:
                uint32_t warnings = 0;

                void fatal(const char* filePath, const uint16_t line, const bgfx::Fatal::Enum code,
                           const char* message) override
                {
                    std::fprintf(stderr, "%s (%u): BGFX FATAL 0x%08x: %s\n", filePath, static_cast<unsigned>(line),
                        static_cast<unsigned>(code), message);
                    if (code != bgfx::Fatal::DebugCheck) {
                        std::abort();
                    }
                }

                void traceVargs(const char* filePath, const uint16_t line, const char* format, va_list args) override
                {
                    if (std::strncmp(format, "BGFX WARN ", 10) == 0) {
                        ++warnings;
                    }
                    std::fprintf(stderr, "%s (%u): ", filePath, static_cast<unsigned>(line));
                    std::vfprintf(stderr, format, args);
                }

                void profilerBegin(const char*, uint32_t, const char*, uint16_t) override {}
                void profilerBeginLiteral(const char*, uint32_t, const char*, uint16_t) override {}
                void profilerEnd() override {}
                uint32_t cacheReadSize(uint64_t) override { return 0; }
                bool cacheRead(uint64_t, void*, uint32_t) override { return false; }
                void cacheWrite(uint64_t, const void*, uint32_t) override {}
                void screenShot(const char*, uint32_t, uint32_t, uint32_t, bgfx::TextureFormat::Enum, const void*,
                                uint32_t, bool) override {}
                void captureBegin(uint32_t, uint32_t, uint32_t, bgfx::TextureFormat::Enum, bool) override {}
                void captureEnd() override {}
                void captureFrame(const void*, uint32_t) override {}
        };

        // One per process, like bgfx: it must outlive bgfx::shutdown.
        TraceCallback& traceCallback()
        {
            static TraceCallback callback;
            return callback;
        }

        bool underDummyDriver()
        {
            const char* driver = SDL_GetCurrentVideoDriver();
            return driver != nullptr && std::strcmp(driver, "dummy") == 0;
        }
    }

    std::unique_ptr<BgfxContext> BgfxContext::create(SDL_Window* window, const uint32_t pixelWidth,
                                                     const uint32_t pixelHeight, const bool vsync, const Device device)
    {
        if (s_alive) {
            std::cerr << "[BgfxContext] ERROR: bgfx is already running; only one context may exist\n";
            return nullptr;
        }

        const bool noop = device == Device::Noop || window == nullptr || underDummyDriver();
        NativeWindow native;
        if (!noop) {
            native = nativeWindowFromSdl(window);
            if (native.nwh == nullptr) {
                return nullptr;
            }
        }

        // renderFrame() before init() on this thread keeps bgfx single-threaded.
        bgfx::renderFrame();

        bgfx::Init init;
        init.type = noop ? bgfx::RendererType::Noop : bgfx::RendererType::Count;   // Count: the platform default
        init.reset = resetFlags(vsync);
        init.callback = &traceCallback();
        traceCallback().warnings = 0;
        init.swapChain.width = std::max(pixelWidth, 1u);
        init.swapChain.height = std::max(pixelHeight, 1u);
        if (!noop) {
            init.platformData.type = native.kind == NativeWindowKind::Wayland
                ? bgfx::NativeWindowHandleType::Wayland
                : bgfx::NativeWindowHandleType::Default;
            init.swapChain.nwh = native.nwh;
            init.swapChain.ndt = native.ndt;
        }

        const bool started = bgfx::init(init);
        const char* failure = started ? bgfxStartFailure(noop, bgfx::getRendererType()) : "bgfx::init failed";
        if (failure != nullptr) {
            std::cerr << "[BgfxContext] ERROR: " << failure << "\n";
            if (started) {
                bgfx::shutdown();
            }
            if (native.metalView != nullptr) {
                SDL_Metal_DestroyView(static_cast<SDL_MetalView>(native.metalView));
            }
            return nullptr;
        }

        std::unique_ptr<BgfxContext> context(
            new BgfxContext(init.swapChain.width, init.swapChain.height, vsync, native.metalView));
        std::cerr << "[BgfxContext] ready: " << context->rendererName() << " " << context->m_width << "x"
                  << context->m_height << "\n";
        return context;
    }

    BgfxContext::BgfxContext(const uint32_t width, const uint32_t height, const bool vsync, void* metalView)
        : m_width(width)
        , m_height(height)
        , m_vsync(vsync)
        , m_metalView(metalView)
    {
        s_alive = true;
    }

    BgfxContext::~BgfxContext()
    {
        bgfx::shutdown();
        if (m_metalView != nullptr) {
            SDL_Metal_DestroyView(static_cast<SDL_MetalView>(m_metalView));
        }
        s_alive = false;
    }

    void BgfxContext::reset(const uint32_t pixelWidth, const uint32_t pixelHeight, const bool vsync)
    {
        m_width = std::max(pixelWidth, 1u);
        m_height = std::max(pixelHeight, 1u);
        m_vsync = vsync;

        // Only the fields set here change; the window handles stay the ones bgfx was given.
        // Noop has no back buffer: its caps accept no swap-chain format, so only the flags change.
        if (bgfx::getRendererType() == bgfx::RendererType::Noop) {
            bgfx::reset(resetFlags(vsync), nullptr);
            return;
        }
        bgfx::SwapChain swapChain;
        swapChain.width = m_width;
        swapChain.height = m_height;
        bgfx::reset(resetFlags(vsync), &swapChain);
    }

    uint32_t BgfxContext::warningCount() const
    {
        return traceCallback().warnings;
    }

    void BgfxContext::frame()
    {
        bgfx::frame();
        ++m_frames;
    }

    uint16_t BgfxContext::allocateView()
    {
        if (m_nextView >= UiView) {
            std::cerr << "[BgfxContext] ERROR: out of bgfx views\n";
            return UiView - 1;
        }
        return m_nextView++;
    }

    uint32_t BgfxContext::frameNumber() const
    {
        return m_frames;
    }

    uint32_t BgfxContext::width() const
    {
        return m_width;
    }

    uint32_t BgfxContext::height() const
    {
        return m_height;
    }

    bool BgfxContext::vsync() const
    {
        return m_vsync;
    }

    const char* BgfxContext::rendererName() const
    {
        return bgfx::getRendererName(bgfx::getRendererType());
    }

}
