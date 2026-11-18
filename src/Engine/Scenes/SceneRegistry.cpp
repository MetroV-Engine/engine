#include "Engine/Scenes/SceneRegistry.hpp"

bool SceneRegistry::add(const std::string& id, Factory factory)
{
    if (id.empty() || !factory)
        return false;
    return _factories.emplace(id, std::move(factory)).second;
}

bool SceneRegistry::remove(const std::string& id)
{
    return _factories.erase(id) > 0;
}

bool SceneRegistry::has(const std::string& id) const
{
    return _factories.find(id) != _factories.end();
}

std::unique_ptr<Scene> SceneRegistry::create(const std::string& id) const
{
    const auto it = _factories.find(id);
    if (it == _factories.end())
        return nullptr;
    return it->second();
}
