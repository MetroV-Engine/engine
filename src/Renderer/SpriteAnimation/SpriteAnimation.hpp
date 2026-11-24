/*
    Created on: 10/4/26
    Filename: SpriteAnimation.hpp
    Description: Frame timing over a single-row sprite sheet: backend-free.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#ifndef RENDERER_WRAPPER_SPRITEANIMATION_HPP
#define RENDERER_WRAPPER_SPRITEANIMATION_HPP

#include "Renderer/RendererTypes.hpp"

#include <cstdint>

namespace gfx {

    // Equal frames laid out left to right on the first row of a sheet.
    class SpriteAnimation {
        public:
            // framesPerSecond <= 0 freezes the animation on its current frame.
            SpriteAnimation(Vec2 frameSize, uint32_t frameCount, float framesPerSecond);

            void update(float deltaSeconds);

            // Holds this frame; the next update() resumes playing from it.
            void stop(uint32_t frame);

            uint32_t frame() const;
            Rect sourceRect() const;

        private:
            Vec2 m_frameSize;
            uint32_t m_frameCount = 1;
            float m_frameDuration = 0.0f;
            float m_elapsed = 0.0f;
            uint32_t m_frame = 0;
    };

}

#endif //RENDERER_WRAPPER_SPRITEANIMATION_HPP
