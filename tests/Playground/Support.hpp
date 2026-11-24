#pragma once

#include "Components/Animator.hpp"
#include "Components/PlayerController.hpp"
#include "Components/Sprite.hpp"
#include "Components/Transform.hpp"
#include "Input/InputState.hpp"

#include "ECS/ECS.hpp"

namespace PlaygroundTest {

// The bull as the level spawns it: feet on the ground at x 120.
inline ECS::Entity spawnBull(ECS::Registry &world)
{
    const ECS::Entity bull = world.spawnEntity("Bull");
    world.addComponent(bull, Transform{Vec2f{120.0f, 600.0f}, 0.0f});
    world.addComponent(bull, PlayerController{});
    Sprite sprite;
    sprite.sheet = "walk.png";
    world.addComponent(bull, sprite);
    Animator animator;
    animator.walk = AnimationClip{"walk.png", 8, 12.0f};
    animator.idle = AnimationClip{"idle.png", 12, 6.0f};
    animator.jump = AnimationClip{"jump.png", 3, 0.0f};
    world.addComponent(bull, animator);
    world.setResource<InputState>();
    return bull;
}

inline InputState &input(ECS::Registry &world)
{
    return world.getResource<InputState>();
}

} // namespace PlaygroundTest
