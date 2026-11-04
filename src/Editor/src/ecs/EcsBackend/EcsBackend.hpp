/*
    Created on: 03/11/2026
    Filename: EcsBackend.hpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#pragma once

#include "ecs/EcsScene/EcsScene.hpp"
#include "ecs/EcsSimulation/EcsSimulation.hpp"
#include "scene/SceneBackend.hpp"

// The editor on an ECS world: the Hierarchy and the Inspector show its
// entities and reflected components (EcsScene), Play runs its systems and
// Stop restores it (EcsSimulation). The Viewport keeps its placeholder until
// the engine renders; a game can draw there with Editor::setViewportRenderer.
//
// On creation, failures of the reflection's automatic registration (two
// types reflected under the same name...) are logged as errors.
class EcsBackend : public SceneBackend
{
public:
    // world and systems must outlive the backend.
    EcsBackend(ECS::Registry &world, ECS::SystemManager &systems);

    HierarchyModel &hierarchy() override { return _scene; }
    PropertyModel &properties() override { return _scene; }
    Simulation &simulation() override { return _simulation; }

    // "components": lists the component types the Inspector can add.
    void registerCommands(CommandRegistry &commands) override;
    std::string description() const override;

private:
    EcsScene _scene;
    EcsSimulation _simulation;
    const ECS::SystemManager &_systems;
};
