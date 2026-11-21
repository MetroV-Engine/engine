/*
    Created on: 11/17/26
    Filename: BgfxContext.hpp
    Description: The process-wide bgfx: init on a window, reset, frame, views; one owner at a time.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#ifndef RENDERER_WRAPPER_BGFXCONTEXT_HPP
#define RENDERER_WRAPPER_BGFXCONTEXT_HPP

#include <cstdint>
#include <memory>

struct SDL_Window;

namespace gfx {

    // bgfx is global: whoever owns the window owns this, and everything drawing with bgfx shares it.
    class BgfxContext {
        public:
            enum class Device : uint8_t { Auto, Noop };

            // Reserved for the UI: the highest id, so bgfx draws it after every game view.
            static constexpr uint16_t UiView = 255;

            // Returns nullptr on failure, after logging the reason; a second live context is a failure.
            // Auto without a window, or under SDL's "dummy" driver, starts the Noop device: there is no native handle.
            static std::unique_ptr<BgfxContext> create(SDL_Window* window, uint32_t pixelWidth, uint32_t pixelHeight,
                                                       bool vsync, Device device = Device::Auto);

            ~BgfxContext();

            BgfxContext(const BgfxContext&) = delete;
            BgfxContext& operator=(const BgfxContext&) = delete;

            void reset(uint32_t pixelWidth, uint32_t pixelHeight, bool vsync);
            void frame();

            // 0, 1, 2…; never UiView.
            uint16_t allocateView();

            uint32_t frameNumber() const;
            uint32_t width() const;
            uint32_t height() const;
            bool vsync() const;
            const char* rendererName() const;
            // bgfx warnings since this context started (debug builds only: release bgfx emits none).
            uint32_t warningCount() const;

        private:
            BgfxContext(uint32_t width, uint32_t height, bool vsync, void* metalView);

            inline static bool s_alive = false;

            uint32_t m_width = 0;
            uint32_t m_height = 0;
            bool m_vsync = false;
            void* m_metalView = nullptr;
            uint32_t m_frames = 0;
            uint16_t m_nextView = 0;
    };

}

#endif //RENDERER_WRAPPER_BGFXCONTEXT_HPP
