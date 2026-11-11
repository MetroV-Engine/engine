#pragma once

#include <string>

#include "ECS/ECS.hpp"

/**
 * @brief A world: its own registry and systems.
 */
class Scene {
    public:
        explicit Scene(std::string name = "Scene") : _name(std::move(name)) {}
        virtual ~Scene() = default;

        Scene(const Scene&) = delete;
        Scene& operator=(const Scene&) = delete;

        const std::string& name() const { return _name; }
        ECS::Registry& registry() { return _registry; }
        ECS::SystemManager& systems() { return _systems; }

        /** @brief Runs every system of the scene. */
        virtual void update(double dt) { _systems.update(_registry, dt); }

        /** @brief Lifecycle, called by the SceneManager. */
        virtual void onEnter() {}
        virtual void onExit() {}
        virtual void onPause() {}
        virtual void onResume() {}

        /** @brief If true, the scene keeps updating when another scene is above it. */
        bool updatesWhenCovered = false;

    private:
        std::string _name;
        ECS::Registry _registry;
        ECS::SystemManager _systems;
};
