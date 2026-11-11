#pragma once

#include <memory>
#include <vector>

#include "Engine/Scenes/Scene.hpp"

/**
 * @brief A stack of scenes: the top one is active.
 */
class SceneManager {
    public:
        ~SceneManager() { clear(); }

        void push(std::unique_ptr<Scene> scene);
        void pop();
        void clear();

        /** @brief Updates the top scene, and the covered ones that ask for it. */
        void update(double dt);

        Scene* top() const { return _scenes.empty() ? nullptr : _scenes.back().get(); }
        bool empty() const { return _scenes.empty(); }

    private:
        std::vector<std::unique_ptr<Scene>> _scenes;
};
