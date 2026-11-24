/*
    Created on: 11/17/26
    Filename: CameraFollowSystem.cpp
    Description: Keeps each Camera on the x of the entity it follows.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "CameraFollowSystem.hpp"

#include "Components/Camera.hpp"
#include "Components/Transform.hpp"

void CameraFollowSystem::update(ECS::Registry &world, double)
{
    for (auto [camera, cameraTransform, settings] : world.view<Transform, Camera>()) {
        if (settings.followTarget.empty())
            continue;
        for (auto [target, name, targetTransform] : world.view<ECS::Name, Transform>()) {
            // Only x: following the hops would bob the whole view.
            if (name.value == settings.followTarget && target != camera) {
                cameraTransform.position.x = targetTransform.position.x;
                break;
            }
        }
    }
}
