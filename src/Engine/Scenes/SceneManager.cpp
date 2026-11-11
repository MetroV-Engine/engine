#include "Engine/Scenes/SceneManager.hpp"

void SceneManager::push(std::unique_ptr<Scene> scene)
{
    if (!scene)
        return;
    if (!_scenes.empty())
        _scenes.back()->onPause();
    _scenes.push_back(std::move(scene));
    _scenes.back()->onEnter();
}

void SceneManager::pop()
{
    if (_scenes.empty())
        return;
    _scenes.back()->onExit();
    _scenes.pop_back();
    if (!_scenes.empty())
        _scenes.back()->onResume();
}

void SceneManager::clear()
{
    while (!_scenes.empty()) {
        _scenes.back()->onExit();
        _scenes.pop_back();
    }
}

void SceneManager::update(double dt)
{
    for (std::size_t i = 0; i < _scenes.size(); ++i) {
        const bool isTop = (i + 1 == _scenes.size());
        if (isTop || _scenes[i]->updatesWhenCovered)
            _scenes[i]->update(dt);
    }
}
