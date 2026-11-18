#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "Engine/Scenes/Scene.hpp"
#include "Engine/Scenes/SceneRegistry.hpp"

/**
 * @brief A stack of scenes: the top one is active.
 */
class SceneManager {
    public:
        ~SceneManager() { clear(); }

        /** @brief Pushes a scene. A null scene is ignored. */
        void push(std::unique_ptr<Scene> scene);

        /** @brief Ignored, like any null scene. Keeps push(nullptr) unambiguous. */
        void push(std::nullptr_t) {}

        /**
         * @brief Creates the scene registered under `id` and pushes it.
         * @return false (nothing pushed) when the id is unknown.
         */
        bool push(const std::string& id);

        void pop();
        void clear();

        /** @brief Updates the top scene, and the covered ones that ask for it. */
        void update(double dt);

        /** @brief The factories used by push(id). */
        SceneRegistry& registry() { return _registry; }
        const SceneRegistry& registry() const { return _registry; }

        /**
         * @brief Sets a function called on every pushed scene before its
         *        onEnter() (the Engine uses it to inject its services).
         */
        void setInitializer(std::function<void(Scene&)> initializer) { _initializer = std::move(initializer); }

        /** @brief Calls `fn` on every scene, bottom to top. */
        void forEach(const std::function<void(Scene&)>& fn) const;

        Scene* top() const { return _scenes.empty() ? nullptr : _scenes.back().get(); }
        bool empty() const { return _scenes.empty(); }

    private:
        std::vector<std::unique_ptr<Scene>> _scenes;
        SceneRegistry _registry;
        std::function<void(Scene&)> _initializer;
};
