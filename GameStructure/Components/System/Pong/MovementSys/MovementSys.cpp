#include "Entity/Base/Position/Position.hpp"
#include "Entity/Base/Velocity/Velocity.hpp"
#include "Entity/Base/Bounds/Bounds.hpp"
#include "Entity/Base/ScreenLocked/ScreenLocked.hpp"
#include "MovementSys.hpp"
#include "ProjectContext.hpp"

namespace ECS::System {
    void MovementSys::update(registry &r, float dt, SDL_Window *win)
    {
        auto *pos = r.get_if<Entity::Position>();
        auto *vel = r.get_if<Entity::Velocity>();
        auto *bounds = r.get_if<Entity::Bounds>();
        auto *locked = r.get_if<Entity::ScreenLocked>();
        if (!pos || !vel || pos->size() == 0 || vel->size() == 0) return;

        float screenW = Editor::ProjectContext::getInstance().screenWidth();
        float screenH = Editor::ProjectContext::getInstance().screenHeight();

        for (auto [p, v, entity] : zipper(*pos, *vel)) {
            p._x += v._vx * dt;
            p._y += v._vy * dt;

            if (locked && locked->has(entity) && locked->get(entity)._locked &&
                bounds && bounds->has(entity)) {
                auto &b = bounds->get(entity);
                if (p._x < 0.f) p._x = 0.f;
                if (p._y < 0.f) p._y = 0.f;
                if (p._x + b._width > screenW) p._x = screenW - b._width;
                if (p._y + b._height > screenH) p._y = screenH - b._height;
            }
        }
    }
}
