#pragma once

#include <string>

namespace ECS {
    class Registry;

    /**
     * @brief A piece of game logic run every frame by a SystemManager.
     *
     * Implement update(); init() and shutdown() are optional hooks.
     */
    class ISystem {
        public:
            virtual ~ISystem() = default;

            /** @brief Called once by SystemManager::addSystem, right after construction. */
            virtual void init(ECS::Registry&) {}

            /** @brief Called once by SystemManager::removeSystem, right before destruction. */
            virtual void shutdown(ECS::Registry&) {}

            /** @brief Called once per frame by SystemManager::update, under its own deferred-mutation scope. */
            virtual void update(ECS::Registry &reg, double dt = 0.0) = 0;

            virtual const std::string name() const = 0;
    };
}
