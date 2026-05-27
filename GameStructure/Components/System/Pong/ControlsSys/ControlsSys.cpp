#include "ControlsSys.hpp"
#include "Entity/Base/Position/Position.hpp"
#include "Entity/Base/Velocity/Velocity.hpp"
#include "Entity/Base/Controlable/Controlable.hpp"
#include "Entity/Pong/Pad/Pad.hpp"
#include "ProjectContext.hpp"

namespace ECS::System {
    void ControlsSys::update(registry &r, float deltaTime, SDL_Window *win)
    {
        checkPlayer1(r, win);
    }

    void ControlsSys::checkPlayer1(registry &r, SDL_Window *win)
    {
        const auto positions = r.get_if<Entity::Position>();
        const auto velocities = r.get_if<Entity::Velocity>();
        const auto controlables = r.get_if<Entity::Controlable>();
        if (!positions || !velocities || !controlables) return;

        for (auto [pos, vel, ctrl, entity]: zipper(*positions, *velocities, *controlables)) {
            if (!ctrl._canMove) continue;

            auto upKey = ctrl._controlsMap[Entity::Moves::UP];
            auto downKey = ctrl._controlsMap[Entity::Moves::DOWN];
            auto leftKey = ctrl._controlsMap[Entity::Moves::LEFT];
            auto rightKey = ctrl._controlsMap[Entity::Moves::RIGHT];

            if (upKey != 0 && Proto::isKeyPressed(upKey)) {
                vel._vy = -vel._speedY;
            } else if (downKey != 0 && Proto::isKeyPressed(downKey)) {
                vel._vy = vel._speedY;
            } else {
                vel._vy = 0.f;
            }

            if (leftKey != 0 && Proto::isKeyPressed(leftKey)) {
                vel._vx = -vel._speedX;
            } else if (rightKey != 0 && Proto::isKeyPressed(rightKey)) {
                vel._vx = vel._speedX;
            } else {
                vel._vx = 0.f;
            }
        }
    }
}
