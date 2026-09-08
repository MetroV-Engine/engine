/*
    Created on: 09/03/2026
    Filename: SaveManager.cpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "SaveManager.hpp"
#include <fstream>
#include <iostream>
#include <unordered_map>
#include "ComponentRegistry.hpp"
#include "ComponentsInit.hpp"
#include "Entity/Moves.hpp"
#include "ProjectContext.hpp"

/*
 * This part of code is horrible, it must be refactored after the end of the prototype !
 */

namespace Editor
{
    void SaveManager::load(ECS::registry& reg)
    {
        std::ifstream input(_saveFile);
        if (!input.is_open())
            return;
        try {
            _j = json::parse(input);
            input.close();
        } catch (const json::parse_error& e) {
            input.close();
            return;
        }

        if (_j.contains("screen")) {
            auto& ctx = Editor::ProjectContext::getInstance();
            ctx.setScreenSize(
                _j["screen"].value("width", 1920.0f),
                _j["screen"].value("height", 1080.0f)
            );
        }

        for (auto& entityData : _j.at("entities")) {
            ECS::Entity::entity e = reg.spawn_entity();

            // Set entity name if provided, otherwise assign a default name based on the entity ID
            if (entityData.contains("name"))
                reg.set_entity_name(e, entityData.at("name").get<std::string>());
            else
                reg.set_entity_name(e, "Entity_" + std::to_string(static_cast<size_t>(e)));

            // Load components from the data file for each entity
            auto& compRegistry = ECS::Component::ComponentRegistry::getInstance();
            for (auto& compData : entityData.at("components")) {
                std::string compType = compData.at("type").get<std::string>();
                const nlohmann::json& data = compData.at("data");

                // 1. Emplace a default-constructed component into the registry
                if (!compRegistry.addComponentToEntity(compType, reg, e)) {
                    std::cerr << "Unknown component type: " << compType << std::endl;
                    continue;
                }

                // 2. Retrieve the live pointer and fill it with JSON data
                auto components = reg.get_components_for(static_cast<size_t>(e));
                for (IComponent* comp : components) {
                    if (comp->getName() == compType) {
                        comp->fromJson(data);
                        break;
                    }
                }
            }
        }
    }

    void SaveManager::save(ECS::registry& reg)
    {
        json output;
        auto entities = reg.get_all_entities();
        output["total_entities"] = entities.size();
        output["entities"] = json::array();

        auto& ctx = Editor::ProjectContext::getInstance();
        output["screen"] = {{"width", ctx.screenWidth()}, {"height", ctx.screenHeight()}};

        for (auto& e : entities) {
            json entityJson;
            entityJson["name"] = reg.get_entity_name(e);
            entityJson["components"] = json::array();

            for (IComponent* comp : reg.get_components_for(static_cast<size_t>(e))) {
                json compJson;
                compJson["type"] = comp->getName();
                compJson["data"] = json::object();

                for (auto& field : comp->getFields()) {
                    json& data = compJson["data"];
                    std::visit([&](auto* ptr)
                    {
                        using T = std::decay_t<decltype(*ptr)>;

                        if constexpr (std::is_same_v<T, float>) {
                            data[field.name] = *ptr;
                        }
                        else if constexpr (std::is_same_v<T, int>) {
                            data[field.name] = *ptr;
                        }
                        else if constexpr (std::is_same_v<T, bool>) {
                            data[field.name] = *ptr;
                        }
                        else if constexpr (std::is_same_v<T, std::string>) {
                            data[field.name] = *ptr;
                        }
                        else if constexpr (std::is_same_v<T, Proto::Vec2f>) {
                            data[field.name] = {{"x", ptr->x}, {"y", ptr->y}};
                        }
                        else if constexpr (std::is_same_v<T, Proto::Color>) {
                            data[field.name] = {
                                {"r", ptr->r}, {"g", ptr->g},
                                {"b", ptr->b}, {"a", ptr->a}
                            };
                        }
                        else if constexpr (std::is_same_v<T, std::map<ECS::Entity::Moves, Proto::Key>>) {
                            static const std::unordered_map<ECS::Entity::Moves, std::string> moveNames = {
                                {ECS::Entity::UP, "UP"},
                                {ECS::Entity::DOWN, "DOWN"},
                                {ECS::Entity::LEFT, "LEFT"},
                                {ECS::Entity::RIGHT, "RIGHT"}
                            };
                            json layout = json::object();
                            for (auto& [move, key] : *ptr)
                                layout[moveNames.at(move)] = static_cast<int>(key);
                            data[field.name] = layout;
                        }
                    }, field.valuePtr);
                }
                entityJson["components"].push_back(compJson);
            }
            output["entities"].push_back(entityJson);
        }

        std::ofstream out(_saveFile);
        if (!out.is_open())
            throw std::runtime_error("Cannot open save file for writing: " + _saveFile);
        out << output.dump(2);
    }
} // Editor
