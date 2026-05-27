/*
    Created on: 3/5/26
    Filename: ComponentRegistry.hpp
    Description: Component registry system for dynamic component discovery

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#ifndef PROTO_COMPONENTREGISTRY_HPP
#define PROTO_COMPONENTREGISTRY_HPP

#include <string>
#include <unordered_map>
#include <vector>
#include <functional>
#include <memory>
#include <typeindex>

#include "IComponent.hpp"
#include "Registry.hpp"
#include "Entity.hpp"

namespace ECS::Component {

    // Structure to hold component metadata
    struct ComponentInfo {
        std::string name;
        std::function<std::unique_ptr<IComponent>()> factory;
        std::function<void(ECS::registry&, const ECS::Entity::entity&)> addToEntity;
        std::function<void(ECS::registry&, const ECS::Entity::entity&)> removeFromEntity;
        std::type_index typeIndex;

        ComponentInfo(const std::string& n,
                     std::function<std::unique_ptr<IComponent>()> f,
                     std::function<void(ECS::registry&, const ECS::Entity::entity&)> add,
                     std::function<void(ECS::registry&, const ECS::Entity::entity&)> remove,
                     std::type_index ti)
            : name(n), factory(std::move(f)), addToEntity(std::move(add)), removeFromEntity(std::move(remove)), typeIndex(ti) {}
    };

    // Registry singleton that holds all available component types
    class ComponentRegistry {
    public:
        static ComponentRegistry& getInstance() {
            static ComponentRegistry instance;
            return instance;
        }

        // Register a component type with its factory functions
        template<typename ComponentType>
        void registerComponent(const std::string& name) {
            auto factory = []() -> std::unique_ptr<IComponent> {
                return std::make_unique<ComponentType>();
            };

            auto addToEntity = [](ECS::registry& reg, const ECS::Entity::entity& entity) {
                reg.register_component<ComponentType>();
                reg.emplace_component<ComponentType>(entity);
            };

            auto removeFromEntity = [](ECS::registry& reg, const ECS::Entity::entity& entity) {
                reg.remove_component<ComponentType>(entity);
            };

            _components.emplace(name, ComponentInfo(
                name,
                factory,
                addToEntity,
                removeFromEntity,
                std::type_index(typeid(ComponentType))
            ));
        }

        // Get all registered component names
        std::vector<std::string> getComponentNames() const {
            std::vector<std::string> names;
            names.reserve(_components.size());
            for (const auto& [name, info] : _components) {
                names.push_back(name);
            }
            return names;
        }

        // Get component info by name
        const ComponentInfo* getComponentInfo(const std::string& name) const {
            auto it = _components.find(name);
            return (it != _components.end()) ? &it->second : nullptr;
        }

        // Create a component instance by name
        std::unique_ptr<IComponent> createComponent(const std::string& name) const {
            auto it = _components.find(name);
            if (it != _components.end()) {
                return it->second.factory();
            }
            return nullptr;
        }

        // Add component to entity by name
        bool addComponentToEntity(const std::string& name, ECS::registry& reg, const ECS::Entity::entity& entity) const {
            auto it = _components.find(name);
            if (it != _components.end()) {
                it->second.addToEntity(reg, entity);
                return true;
            }
            return false;
        }

        // Remove component from entity by name
        bool removeComponentFromEntity(const std::string& name, ECS::registry& reg, const ECS::Entity::entity& entity) const {
            auto it = _components.find(name);
            if (it != _components.end()) {
                it->second.removeFromEntity(reg, entity);
                return true;
            }
            return false;
        }

    private:
        ComponentRegistry() = default;
        std::unordered_map<std::string, ComponentInfo> _components;
    };

    // Registration helper class for automatic registration
    template<typename ComponentType>
    struct ComponentRegistrar {
        ComponentRegistrar(const std::string& name) {
            ComponentRegistry::getInstance().registerComponent<ComponentType>(name);
        }
    };
}

// Macro for component registration
#define REGISTER_COMPONENT(ComponentType, ComponentName) \
    namespace { \
        static const ECS::Component::ComponentRegistrar<ComponentType> \
            CONCAT(_registrar_, __COUNTER__)(ComponentName); \
    }

// Helper macro for token concatenation
#define CONCAT(a, b) CONCAT_IMPL(a, b)
#define CONCAT_IMPL(a, b) a##b

#endif //PROTO_COMPONENTREGISTRY_HPP


