/*
    Created on: 09/11/2026
    Filename: EntityDropTarget.hpp
    Description: Lets a panel receive the entity templates dragged from the Entity Palette

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#pragma once

#include "scene/PlayController/PlayController.hpp"

#include <cstdint>
#include <functional>

// Shared by the panels that accept entity templates. Nothing is accepted
// unless the game is stopped: what is created during Play disappears on Stop.
class EntityDropTarget
{
public:
    using Handler = std::function<void(std::uint32_t templateIndex)>;

    // play must outlive the target.
    EntityDropTarget(const PlayController &play, Handler onDrop);

    // Makes the last drawn item a drop target. Calls the handler, during the
    // frame, when a template is dropped on it.
    void accept() const;

private:
    const PlayController &_play;
    Handler _onDrop;
};
