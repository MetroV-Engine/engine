#include <boost/ut.hpp>

#include "Support/Expect.hpp"
#include "Renderer/SpriteAnimation/SpriteAnimation.hpp"

using namespace boost::ut;

namespace {

    constexpr gfx::Vec2 Frame{ 512.0f, 512.0f };

    // 4 fps: a frame lasts exactly 0.25 s, which floats represent without rounding.
    constexpr float Fps = 4.0f;
    constexpr float FrameTime = 0.25f;

}

void run_sprite_animation_tests()
{
    "sprite animation: starts on first frame"_test = [] {
        const gfx::SpriteAnimation animation(Frame, 8, Fps);
        const gfx::Rect source = animation.sourceRect();

        expect(animation.frame() == 0u);
        expect(test::floatEq(source.x, 0.0f));
        expect(test::floatEq(source.y, 0.0f));
        expect(test::floatEq(source.w, 512.0f));
        expect(test::floatEq(source.h, 512.0f));
    };

    "sprite animation: advances once aframe duration has elapsed"_test = [] {
        gfx::SpriteAnimation animation(Frame, 8, Fps);

        animation.update(FrameTime * 0.5f);
        expect(animation.frame() == 0u);

        animation.update(FrameTime * 0.5f);
        expect(animation.frame() == 1u);
    };

    "sprite animation: source rect follows frame"_test = [] {
        gfx::SpriteAnimation animation(Frame, 8, Fps);
        animation.update(FrameTime * 3.0f);

        expect(animation.frame() == 3u);
        expect(test::floatEq(animation.sourceRect().x, 1536.0f));
    };

    "sprite animation: wraps after last frame"_test = [] {
        gfx::SpriteAnimation animation(Frame, 8, Fps);
        animation.update(FrameTime * 9.0f);

        expect(animation.frame() == 1u);
    };

    "sprite animation: large delta skips frames and keeps remainder"_test = [] {
        gfx::SpriteAnimation animation(Frame, 8, Fps);

        animation.update(FrameTime * 3.5f);
        expect(animation.frame() == 3u);

        animation.update(FrameTime * 0.5f);
        expect(animation.frame() == 4u);
    };

    "sprite animation: stop holds frame and restarts timing"_test = [] {
        gfx::SpriteAnimation animation(Frame, 8, Fps);
        animation.update(FrameTime * 0.5f);

        animation.stop(2);
        expect(animation.frame() == 2u);

        animation.update(FrameTime * 0.5f);     // would have completed a frame without the reset
        expect(animation.frame() == 2u);

        animation.update(FrameTime * 0.5f);
        expect(animation.frame() == 3u);
    };

    "sprite animation: stop wraps out of range frame"_test = [] {
        gfx::SpriteAnimation animation(Frame, 8, Fps);
        animation.stop(10);

        expect(animation.frame() == 2u);
    };

    "sprite animation: non positive fps freezes"_test = [] {
        for (const float fps : { 0.0f, -5.0f }) {
            gfx::SpriteAnimation animation(Frame, 8, fps);
            animation.update(10.0f);

            expect(animation.frame() == 0u);
        }
    };

    "sprite animation: zero frame count behaves as single frame"_test = [] {
        gfx::SpriteAnimation animation(Frame, 0, Fps);

        animation.update(FrameTime * 5.0f);
        expect(animation.frame() == 0u);

        animation.stop(5);
        expect(animation.frame() == 0u);
    };
}
