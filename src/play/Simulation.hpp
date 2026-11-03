/*
    Created on: 30/03/2026
    Filename: Simulation.hpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#pragma once

// What the editor runs in play mode. The PlayController calls begin() on
// Play, update() every frame while playing, and end() on Stop.
//
// With the engine's ECS, update() will call SystemManager::update(world, dt),
// begin() will save the world and end() restore it, so that what happens in
// play mode never changes the edited level.
class Simulation
{
public:
    virtual ~Simulation() = default;

    // Saves the edited state and prepares the game.
    virtual void begin() = 0;
    // Advances the game by dt seconds (already scaled by the time scale).
    virtual void update(double dt) = 0;
    // Stops the game and restores the edited state.
    virtual void end() = 0;
};
