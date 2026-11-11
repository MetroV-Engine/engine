/*
    Created on: 30/03/2026
    Filename: DemoSimulation.cpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "DemoSimulation.hpp"

#include <cmath>

namespace {

using ObjectId = HierarchyModel::ObjectId;

template <typename T>
bool read(const DemoScene &scene, ObjectId id, const char *component, const char *path, T &out)
{
    const PropertyValue v = scene.get(id, component, path);
    if (const T *typed = std::get_if<T>(&v)) {
        out = *typed;
        return true;
    }
    return false;
}

} // namespace

void DemoSimulation::begin()
{
    _snapshot = _scene;
    _time = 0.0;
    _velocities.clear();
    _startPositions.clear();
    for (ObjectId id : _scene.allObjects()) {
        Vec2 position;
        if (read(_scene, id, PositionComponent, PositionProperty, position))
            _startPositions[id] = position;
    }
}

void DemoSimulation::update(double dt)
{
    _time += dt;
    const float step = static_cast<float>(dt);

    for (ObjectId id : _scene.allObjects()) {
        Vec2 position;
        if (!read(_scene, id, PositionComponent, PositionProperty, position))
            continue;
        const Vec2 start = _startPositions.contains(id) ? _startPositions[id] : position;

        // Falling bodies: gravity, then they stop on the ground.
        int bodyType = 0;
        float gravityScale = 1.0f;
        if (read(_scene, id, "RigidBody", "bodyType", bodyType) && bodyType == 1) {
            read(_scene, id, "RigidBody", "gravityScale", gravityScale);
            Vec2 size{0, 0};
            read(_scene, id, "BoxCollider", "size", size);
            Vec2 &velocity = _velocities[id];
            velocity.y += Gravity * gravityScale * step;
            position.y += velocity.y * step;
            if (position.y + size.y * 0.5f > GroundY) {
                position.y = GroundY - size.y * 0.5f;
                velocity.y = 0.0f;
            }
        }

        // Patrolling enemies walk back and forth around their start.
        int behavior = 0;
        float range = 0.0f;
        if (read(_scene, id, "EnemyAI", "behavior", behavior) && behavior == 1) {
            read(_scene, id, "EnemyAI", "detectionRange", range);
            position.x = start.x + std::sin(static_cast<float>(_time) * 1.5f + static_cast<float>(id)) * range * 0.5f;
        }

        // Collectibles float up and down.
        int kind = 0;
        if (read(_scene, id, "Collectible", "kind", kind))
            position.y = start.y + std::sin(static_cast<float>(_time) * 3.0f + static_cast<float>(id) * 0.7f) * 4.0f;

        _scene.set(id, PositionComponent, PositionProperty, position);
    }
}

void DemoSimulation::end()
{
    _scene = _snapshot;
    _velocities.clear();
    _startPositions.clear();
}
