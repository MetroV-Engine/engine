/*
    Created on: 03/11/2026
    Filename: SceneBackend.hpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#pragma once

#include "scene/EntityTemplate.hpp"
#include "scene/HierarchyModel.hpp"
#include "scene/PropertyModel.hpp"
#include "scene/Selection.hpp"
#include "scene/Simulation.hpp"

#include <imgui.h>

#include <functional>
#include <string>
#include <vector>

class CommandRegistry;

// What the editor works on: the objects listed by the Hierarchy, their
// components edited by the Inspector, the game run by Play and, optionally,
// how to draw it in the Viewport. The Editor only talks to this interface:
//  - DemoBackend: the built-in demo scene, without the engine (--demo);
//  - EcsBackend: an ECS::Registry and its systems (Editor's ECS constructor).
class SceneBackend
{
public:
    // Same signature as ViewportPanel::Renderer.
    using Renderer = std::function<void(ImDrawList *, ImVec2 min, ImVec2 max)>;

    virtual ~SceneBackend() = default;

    virtual HierarchyModel &hierarchy() = 0;
    virtual PropertyModel &properties() = 0;
    virtual Simulation &simulation() = 0;

    // Draws the scene in the Viewport, highlighting the selection. An empty
    // renderer leaves the Viewport's placeholder.
    virtual Renderer renderer(const Selection &) { return {}; }
    // What the Entity Palette offers for this scene.
    virtual std::vector<EntityTemplate> entityTemplates() const { return {{"Empty Entity", "Entity", {}}}; }
    // Console commands specific to this scene.
    virtual void registerCommands(CommandRegistry &) {}
    // One line logged at startup, e.g. "ECS world: 12 entities".
    virtual std::string description() const = 0;
};
