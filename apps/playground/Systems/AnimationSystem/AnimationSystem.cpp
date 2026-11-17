/*
    Created on: 11/17/26
    Filename: AnimationSystem.cpp
    Description: Picks the bull's clip from its controller and advances its frames into its Sprite.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "AnimationSystem.hpp"

#include "Components/Animator.hpp"
#include "Components/PlayerController.hpp"
#include "Components/Sprite.hpp"

#include <cmath>

namespace {

enum Clip
{
    Walk = 0,
    Idle = 1,
    Jump = 2,
};

constexpr int StandingFrame = 2; // walk frame with the legs together: reads as standing
constexpr int RisingFrame = 0;
constexpr int FallingFrame = 1;
constexpr int LandingFrame = 2;

// Advances frame by the whole frames elapsed; a long stall skips frames instead of looping.
void advance(Animator &animator, const AnimationClip &clip, const float dt)
{
    if (clip.fps <= 0.0f || clip.frames <= 0)
        return;
    const float frameDuration = 1.0f / clip.fps;
    animator.elapsed += dt;
    const auto steps = static_cast<int>(std::floor(animator.elapsed / frameDuration));
    animator.elapsed -= static_cast<float>(steps) * frameDuration;
    animator.frame = (animator.frame + steps % clip.frames) % clip.frames;
}

void enter(Animator &animator, const int clip, const int frame)
{
    if (animator.clip == clip)
        return;
    animator.clip = clip;
    animator.frame = frame;
    animator.elapsed = 0.0f;
}

} // namespace

void AnimationSystem::update(ECS::Registry &world, const double dt)
{
    const float step = static_cast<float>(dt);

    for (auto [entity, sprite, animator, controller] : world.view<Sprite, Animator, PlayerController>()) {
        if (controller.airborne || controller.landingLeft > 0.0f) {
            enter(animator, Jump, LandingFrame);
            animator.frame = controller.airborne ? (controller.velocityY < 0.0f ? RisingFrame : FallingFrame) : LandingFrame;
            sprite.sheet = animator.jump.sheet.empty() ? animator.walk.sheet : animator.jump.sheet;
        } else if (controller.walking) {
            enter(animator, Walk, StandingFrame);
            advance(animator, animator.walk, step);
            sprite.sheet = animator.walk.sheet;
        } else if (animator.idle.sheet.empty()) {
            enter(animator, Idle, StandingFrame);
            animator.frame = StandingFrame;
            sprite.sheet = animator.walk.sheet;
        } else {
            // Every stop starts a fresh breath, never halfway through the tail swish.
            enter(animator, Idle, 0);
            advance(animator, animator.idle, step);
            sprite.sheet = animator.idle.sheet;
        }

        sprite.frame = animator.frame;
        sprite.flipX = controller.facingLeft;
    }
}
