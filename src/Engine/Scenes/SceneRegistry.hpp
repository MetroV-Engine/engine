#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

#include "Engine/Scenes/Scene.hpp"

/**
 * @brief Creates scenes from an identifier ("menu", "levels/level2"...), so a
 *        scene can be requested without knowing its C++ type.
 */
class SceneRegistry {
    public:
        using Factory = std::function<std::unique_ptr<Scene>()>;

        /**
         * @brief Registers a factory under an identifier.
         * @return false (nothing registered) when the id is empty, the factory
         *         is empty or the id is already taken.
         */
        bool add(const std::string& id, Factory factory);

        /** @brief Unregisters an identifier. @return false when it is unknown. */
        bool remove(const std::string& id);

        bool has(const std::string& id) const;
        std::size_t size() const { return _factories.size(); }

        /**
         * @brief Creates a new scene.
         * @return The scene, or nullptr when the id is unknown or its factory
         *         returned nothing.
         */
        std::unique_ptr<Scene> create(const std::string& id) const;

    private:
        std::unordered_map<std::string, Factory> _factories;
};
