/*
    Created on: 11/17/26
    Filename: Animator.hpp
    Description: The bull's walk, idle and jump clips and where it is in the current one.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#pragma once

#include "ECS/Reflection/Reflection.hpp"

#include <string>

struct AnimationClip
{
    std::string sheet;          // empty: the clip is missing
    int frames = 1;
    float fps = 0.0f;           // <= 0: frozen
};

// State lives in plain fields, not in a helper object, so Stop can restore it.
struct Animator
{
    AnimationClip walk;
    AnimationClip idle;
    AnimationClip jump;         // three frames: rising, falling, landing
    int clip = 1;               // 0 walk, 1 idle, 2 jump
    int frame = 0;
    float elapsed = 0.0f;
};

E_REFLECT(AnimationClip)
    E_FIELD(sheet, std::string)
    E_FIELD(frames, int)
    E_FIELD(fps, float)
E_END

E_REFLECT(Animator)
    E_FIELD(walk, AnimationClip)
    E_FIELD(idle, AnimationClip)
    E_FIELD(jump, AnimationClip)
    E_FIELD(clip, int)
    E_FIELD(frame, int)
    E_FIELD(elapsed, float)
E_END
