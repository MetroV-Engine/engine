/*
    Created on: 10/4/26
    Filename: SpriteAnimation.cpp
    Description: Frame timing over a single-row sprite sheet: backend-free.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "Renderer/SpriteAnimation/SpriteAnimation.hpp"

namespace gfx {

    SpriteAnimation::SpriteAnimation(const Vec2 frameSize, const uint32_t frameCount, const float framesPerSecond)
        : m_frameSize(frameSize)
        , m_frameCount(frameCount > 0 ? frameCount : 1)
        , m_frameDuration(framesPerSecond > 0.0f ? 1.0f / framesPerSecond : 0.0f)
    {
    }

    void SpriteAnimation::update(const float deltaSeconds)
    {
        if (m_frameDuration <= 0.0f) {
            return;
        }

        m_elapsed += deltaSeconds;

        // A long stall (window drag, breakpoint) skips frames in one step instead of looping.
        const auto steps = static_cast<uint32_t>(m_elapsed / m_frameDuration);
        m_elapsed -= static_cast<float>(steps) * m_frameDuration;
        m_frame = (m_frame + steps % m_frameCount) % m_frameCount;
    }

    void SpriteAnimation::stop(const uint32_t frame)
    {
        m_frame = frame % m_frameCount;
        m_elapsed = 0.0f;
    }

    uint32_t SpriteAnimation::frame() const
    {
        return m_frame;
    }

    Rect SpriteAnimation::sourceRect() const
    {
        return Rect{ static_cast<float>(m_frame) * m_frameSize.x, 0.0f, m_frameSize.x, m_frameSize.y };
    }

}
