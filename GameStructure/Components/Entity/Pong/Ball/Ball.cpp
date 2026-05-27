/*
    Created on: 3/2/26
    Filename: Ball.cpp

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/


#include "Ball.hpp"

#include "Entity/Base/Bounds/Bounds.hpp"
#include "Entity/Base/Position/Position.hpp"
#include "Entity/Base/Velocity/Velocity.hpp"
#include "System/Pong/BallSys/BallSys.hpp"
#include "Entity/Base/Drawable/Drawable.hpp"

namespace ECS::Entity {
    Ball::Ball(registry &reg, std::vector<System::ISystem *> &sys)
    {
        auto ball = reg.spawn_entity();

        reg.set_entity_name(ball, "Ball");
        reg.emplace_component<ECS::Entity::Position>(ball, 960.f, 540.f);
        reg.emplace_component<ECS::Entity::Velocity>(ball, -100.f, 300.f, 100.f);
        reg.emplace_component<ECS::Entity::Bounds>(ball, 50.f, 50.f, Proto::Color::White());
        reg.emplace_component<ECS::Entity::BallTag>(ball);
        reg.emplace_component<ECS::Entity::Drawable>(ball, true);

        sys.push_back(new ECS::System::BallSys);
    }
}
