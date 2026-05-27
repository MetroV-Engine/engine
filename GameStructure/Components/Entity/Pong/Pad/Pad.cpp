/*
    Created on: 3/2/26
    Filename: Pad.cpp

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/


#include "Pad.hpp"

#include "Entity/Base/Bounds/Bounds.hpp"
#include "Entity/Base/Controlable/Controlable.hpp"
#include "Entity/Base/Position/Position.hpp"
#include "Entity/Base/Velocity/Velocity.hpp"
#include "System/Pong/ControlsSys/ControlsSys.hpp"
#include "System/Pong/MovementSys/MovementSys.hpp"
#include "Entity/Base/Drawable/Drawable.hpp"

ECS::Entity::Pad::Pad(ECS::registry &reg,
    std::vector<ECS::System::ISystem *> &sys,
    std::map<Moves, Proto::Key> &controlsMap,
    Position pos,
    Proto::Color color
)
{
    auto player = reg.spawn_entity();
    reg.set_entity_name(player, "Player" + std::to_string(player));
    reg.emplace_component<ECS::Entity::Position>(player, pos);
    reg.emplace_component<ECS::Entity::Velocity>(player, 0.f, 0.f, 500.f);
    reg.emplace_component<ECS::Entity::Bounds>(player, 50.f, 300.f, color);
    reg.emplace_component<ECS::Entity::Controlable>(player, true, controlsMap);
    reg.emplace_component<ECS::Entity::Drawable>(player, true);

    sys.push_back(new ECS::System::ControlsSys);
    sys.push_back(new ECS::System::MovementSys);
}
