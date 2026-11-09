/*
    Created on: 03/11/2026
    Filename: DemoBackend.hpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#pragma once

#include "scene/DemoScene/DemoScene.hpp"
#include "scene/DemoSceneRenderer/DemoSceneRenderer.hpp"
#include "scene/DemoSimulation/DemoSimulation.hpp"
#include "scene/SceneBackend.hpp"

// The built-in demo scene, which needs no engine: an in-memory scene (empty,
// or the MetroidVania demo level), its tiny game and its box renderer.
class DemoBackend : public SceneBackend
{
public:
    explicit DemoBackend(bool withDemoLevel) : _scene(withDemoLevel), _simulation(_scene), _demoLevel(withDemoLevel) {}

    HierarchyModel &hierarchy() override { return _scene; }
    PropertyModel &properties() override { return _scene; }
    Simulation &simulation() override { return _simulation; }

    Renderer renderer(const Selection &selection) override
    {
        return [renderer = DemoSceneRenderer(_scene, selection)](ImDrawList *drawList, ImVec2 min, ImVec2 max) {
            return renderer.draw(drawList, min, max);
        };
    }

    // Every demo object is created with a Transform: templates do not list it.
    std::vector<EntityTemplate> entityTemplates() const override
    {
        return {
            {"Empty Object", "Object", {}},
            {"Sprite", "Sprite", {"Sprite"}},
            {"Enemy", "Enemy", {"Sprite", "BoxCollider", "Health", "EnemyAI"}},
            {"Collectible", "Collectible", {"Sprite", "BoxCollider", "Collectible"}},
            {"Camera", "Camera", {"Camera"}},
        };
    }

    std::string description() const override
    {
        return _demoLevel ? "Editing the built-in demo level" : "Editing an empty built-in demo scene";
    }

private:
    DemoScene _scene;
    DemoSimulation _simulation;
    bool _demoLevel;
};
