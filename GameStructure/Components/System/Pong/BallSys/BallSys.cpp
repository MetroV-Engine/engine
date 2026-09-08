#include "BallSys.hpp"
#include <cmath>
#include <algorithm>
#include <random>
#include "Entity/Base/Bounds/Bounds.hpp"
#include "Entity/Base/Controlable/Controlable.hpp"
#include "Entity/Base/Position/Position.hpp"
#include "Entity/Base/Velocity/Velocity.hpp"
#include "Entity/Pong/Ball/Ball.hpp"
#include "ProjectContext.hpp"

namespace ECS::System {

    void BallSys::update(registry &r, float deltaTime, SDL_Window *win)
    {
        checkWalls(r, win);
        checkCollisionWithPad(r);
        checkOutOfBounds(r, win);
    }

    void BallSys::checkWalls(registry &r, SDL_Window *win)
    {
        const auto ball = r.get_if<Entity::BallTag>();
        const auto positions = r.get_if<Entity::Position>();
        const auto velocities = r.get_if<Entity::Velocity>();
        const auto bounds = r.get_if<Entity::Bounds>();
        if (!ball || !positions || !velocities || !bounds) return;

        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_real_distribution<float> dis(-20.f, 20.f);

        for (auto [b, pos, vel, bound, entity]: zipper(*ball, *positions, *velocities, *bounds)) {
            float screenH = Editor::ProjectContext::getInstance().screenHeight();
            if (pos._y <= 0.f) {
                pos._y = 0.f;
                vel._vy = std::abs(vel._vy);
                vel._vx += dis(gen);
            }
            if (pos._y + bound._height >= screenH) {
                pos._y = screenH - bound._height;
                vel._vy = -std::abs(vel._vy);
                vel._vx += dis(gen);
            }
            if (std::abs(vel._vy) > 600.f) vel._vy *= 0.98f;
        }
    }

    void BallSys::checkCollisionWithPad(registry &r)
    {
        const auto ball = r.get_if<Entity::BallTag>();
        const auto positions = r.get_if<Entity::Position>();
        const auto velocities = r.get_if<Entity::Velocity>();
        const auto bounds = r.get_if<Entity::Bounds>();
        const auto controllables = r.get_if<Entity::Controlable>();
        if (!ball || !positions || !velocities || !bounds || !controllables) return;

        for (auto [b, pos, vel, bound, _]: zipper(*ball, *positions, *velocities, *bounds)) {
            for (auto [ctrl, ctrlPos, ctrlBound, unusedEntity]: zipper(*controllables, *positions, *bounds)) {
                float ballRight = pos._x + bound._width;
                float ballBottom = pos._y + bound._height;
                float padRight = ctrlPos._x + ctrlBound._width;
                float padBottom = ctrlPos._y + ctrlBound._height;

                bool collisionX = ballRight > ctrlPos._x && pos._x < padRight;
                bool collisionY = ballBottom > ctrlPos._y && pos._y < padBottom;

                if (collisionX && collisionY) {
                    float ballCenterY = pos._y + bound._height / 2.f;
                    float padCenterX = ctrlPos._x + ctrlBound._width / 2.f;
                    float padCenterY = ctrlPos._y + ctrlBound._height / 2.f;

                    float overlapLeft = ballRight - ctrlPos._x;
                    float overlapRight = padRight - pos._x;
                    float overlapTop = ballBottom - ctrlPos._y;
                    float overlapBottom = padBottom - pos._y;
                    float minOverlap = std::min({overlapLeft, overlapRight, overlapTop, overlapBottom});

                    if (minOverlap == overlapLeft || minOverlap == overlapRight) {
                        vel._vx = -vel._vx;
                        float hitPos = std::clamp((ballCenterY - padCenterY) / (ctrlBound._height / 2.f), -1.f, 1.f);
                        vel._vy += hitPos * 200.f;
                        pos._x = (minOverlap == overlapLeft) ? ctrlPos._x - bound._width - 1.f : padRight + 1.f;
                        vel._vx *= 1.02f; vel._vy *= 1.02f;
                    } else {
                        vel._vy = -vel._vy;
                        float ballCenterX = pos._x + bound._width / 2.f;
                        float hitPos = std::clamp((ballCenterX - padCenterX) / (ctrlBound._width / 2.f), -1.f, 1.f);
                        vel._vx += hitPos * 100.f;
                        pos._y = (minOverlap == overlapTop) ? ctrlPos._y - bound._height - 1.f : padBottom + 1.f;
                    }

                    float spd = std::sqrt(vel._vx * vel._vx + vel._vy * vel._vy);
                    if (spd < 150.f) { float r = 150.f / spd; vel._vx *= r; vel._vy *= r; }
                    if (spd > 800.f) { float r = 800.f / spd; vel._vx *= r; vel._vy *= r; }
                }
            }
        }
    }

    void BallSys::checkOutOfBounds(registry &r, SDL_Window *win)
    {
        const auto ball = r.get_if<Entity::BallTag>();
        const auto positions = r.get_if<Entity::Position>();
        const auto bounds = r.get_if<Entity::Bounds>();
        const auto velocity = r.get_if<Entity::Velocity>();
        if (!ball || !positions || !bounds || !velocity) return;

        for (auto [b, pos, bound, vel, entity]: zipper(*ball, *positions, *bounds, *velocity)) {
            float screenW = Editor::ProjectContext::getInstance().screenWidth();
            if (pos._x + bound._width < 0.f || pos._x > screenW) {
                pos._x = screenW / 2.f; pos._y = Editor::ProjectContext::getInstance().screenHeight() / 2.f;
                vel._vx = -vel._vx; vel._vy = 300.f;
            }
        }
    }
}
