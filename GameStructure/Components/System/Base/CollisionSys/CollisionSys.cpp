/*
    Created on: 3/6/26
    Filename: CollisionSys.cpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/


#include "CollisionSys.hpp"
#include "Entity/GameEntities.hpp"
#include <algorithm>

namespace ECS::System {

    void CollisionSys::update(registry &r, float deltaTime, SDL_Window *win)
    {
        const auto pos = r.get_if<Entity::Position>();
        const auto col = r.get_if<Entity::Collider>();
        const auto bounds = r.get_if<Entity::Bounds>();

        if (!pos || !col || !bounds) return;

        // Entity-to-entity collision detection using AABB
        for (auto [p1, c1, b1, entity1] : zipper(*pos, *col, *bounds)) {
            for (auto [p2, c2, b2, entity2] : zipper(*pos, *col, *bounds)) {
                if (entity1 == entity2) continue;

                // Skip if neither entity can collide
                if (!c1._canCollide && !c2._canCollide) continue;

                // AABB collision detection
                bool collision = checkAABBCollision(
                    p1._x, p1._y, b1._width, b1._height,
                    p2._x, p2._y, b2._width, b2._height
                );

                if (collision) {
                    handleCollision(p1, c1, b1, entity1, p2, c2, b2, entity2);
                }
            }
        }
    }

    bool CollisionSys::checkAABBCollision(float x1, float y1, float w1, float h1,
                                          float x2, float y2, float w2, float h2)
    {
        return (x1 < x2 + w2 &&
                x1 + w1 > x2 &&
                y1 < y2 + h2 &&
                y1 + h1 > y2);
    }

    void CollisionSys::handleCollision(Entity::Position &pos1, Entity::Collider &col1, Entity::Bounds &bounds1, size_t entity1,
                                       Entity::Position &pos2, Entity::Collider &col2, Entity::Bounds &bounds2, size_t entity2)
    {
        // Calculate overlap distances
        float overlapLeft = (pos1._x + bounds1._width) - pos2._x;
        float overlapRight = (pos2._x + bounds2._width) - pos1._x;
        float overlapTop = (pos1._y + bounds1._height) - pos2._y;
        float overlapBottom = (pos2._y + bounds2._height) - pos1._y;

        // Find minimum overlap to determine separation direction
        float minOverlapX = std::min(overlapLeft, overlapRight);
        float minOverlapY = std::min(overlapTop, overlapBottom);

        // Determine which entities can move
        bool entity1CanMove = col1._canCollide;
        bool entity2CanMove = col2._canCollide;

        // Separate entities based on collision rules
        if (minOverlapX < minOverlapY) {
            // Horizontal separation
            if (entity1CanMove && entity2CanMove) {
                // Both entities can move - split the separation
                if (overlapLeft < overlapRight) {
                    pos1._x -= minOverlapX / 2.0f;
                    pos2._x += minOverlapX / 2.0f;
                } else {
                    pos1._x += minOverlapX / 2.0f;
                    pos2._x -= minOverlapX / 2.0f;
                }
            } else if (entity1CanMove && !entity2CanMove) {
                // Only entity1 can move - move it away from entity2
                if (overlapLeft < overlapRight) {
                    pos1._x -= minOverlapX;
                } else {
                    pos1._x += minOverlapX;
                }
            } else if (!entity1CanMove && entity2CanMove) {
                // Only entity2 can move - move it away from entity1
                if (overlapLeft < overlapRight) {
                    pos2._x += minOverlapX;
                } else {
                    pos2._x -= minOverlapX;
                }
            }
            // If neither can move, no separation occurs
        } else {
            // Vertical separation
            if (entity1CanMove && entity2CanMove) {
                // Both entities can move - split the separation
                if (overlapTop < overlapBottom) {
                    pos1._y -= minOverlapY / 2.0f;
                    pos2._y += minOverlapY / 2.0f;
                } else {
                    pos1._y += minOverlapY / 2.0f;
                    pos2._y -= minOverlapY / 2.0f;
                }
            } else if (entity1CanMove && !entity2CanMove) {
                // Only entity1 can move - move it away from entity2
                if (overlapTop < overlapBottom) {
                    pos1._y -= minOverlapY;
                } else {
                    pos1._y += minOverlapY;
                }
            } else if (!entity1CanMove && entity2CanMove) {
                // Only entity2 can move - move it away from entity1
                if (overlapTop < overlapBottom) {
                    pos2._y += minOverlapY;
                } else {
                    pos2._y -= minOverlapY;
                }
            }
            // If neither can move, no separation occurs
        }

        // Optional: Disable collision for both entities temporarily
        // col1._canCollide = false;
        // col2._canCollide = false;
    }
}
