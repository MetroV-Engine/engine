/*
    Created on: 30/03/2026
    Filename: DemoSimulation.hpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#pragma once

#include "DemoScene.hpp"
#include "play/Simulation.hpp"

#include <unordered_map>

// A tiny game for the demo level, until the engine's ECS runs the real one:
// dynamic RigidBodies fall and land on the ground, enemies with EnemyAI
// patrol, collectibles float. On Stop the scene goes back to its state
// before Play.
class DemoSimulation : public Simulation
{
public:
    // Ground height (y grows downwards) and gravity of the demo world.
    static constexpr float GroundY = 480.0f;
    static constexpr float Gravity = 900.0f;

    explicit DemoSimulation(DemoScene &scene) : _scene(scene) {}

    void begin() override;
    void update(double dt) override;
    void end() override;

private:
    DemoScene &_scene;
    DemoScene _snapshot;
    double _time = 0.0;
    // Per-object state that is not a component value.
    std::unordered_map<HierarchyModel::ObjectId, Vec2> _velocities;
    std::unordered_map<HierarchyModel::ObjectId, Vec2> _startPositions;
};
