/*
    Created on: 11/17/26
    Filename: PlayerSystem.cpp
    Description: Walks and hops every entity with a PlayerController, from the InputState resource.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "PlayerSystem.hpp"

#include "Components/PlayerController.hpp"
#include "Components/Transform.hpp"
#include "Input/InputState.hpp"

#include <utility>

void PlayerSystem::update(ECS::Registry &world, const double dt)
{
    InputState input;
    if (world.hasResource<InputState>()) {
        // Consumed: if nothing refills it (the Viewport hidden behind a tab), no key stays held.
        input = std::exchange(world.getResource<InputState>(), InputState{});
    }
    const float step = static_cast<float>(dt);
    const int direction = (input.right ? 1 : 0) - (input.left ? 1 : 0);

    for (auto [entity, transform, controller] : world.view<Transform, PlayerController>()) {
        Vec2f &feet = transform.position;

        if (input.jumpPressed && !controller.airborne) {
            controller.airborne = true;
            controller.velocityY = -controller.jumpSpeed;
            controller.landingLeft = 0.0f;
        }

        if (direction != 0) {
            controller.facingLeft = direction < 0;
            feet.x += static_cast<float>(direction) * controller.walkSpeed * step;
        }

        if (controller.airborne) {
            controller.velocityY += controller.gravity * step;
            feet.y += controller.velocityY * step;
            // Only while falling: on the frame of the jump the feet are still on the ground.
            if (controller.velocityY > 0.0f && feet.y >= controller.groundY) {
                feet.y = controller.groundY;
                controller.velocityY = 0.0f;
                controller.airborne = false;
                controller.landingLeft = controller.landingSeconds;
            }
            continue;
        }

        if (controller.landingLeft > 0.0f)
            controller.landingLeft -= step;
        controller.walking = direction != 0;
    }
}
