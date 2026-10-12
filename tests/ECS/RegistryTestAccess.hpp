#pragma once

#include <initializer_list>
#include <vector>

#include "ECS/ComponentId.hpp"
#include "ECS/ComponentPool.hpp"
#include "ECS/Entity.hpp"
#include "ECS/Registry.hpp"

namespace ECS::TestAccess {
    /** @brief Reaches the Registry's private ComponentId-based access, which only reflect::RegistryInspector (a friend) may use in production. */
    struct RegistryIdAccess {
        static void ensurePoolById(Registry& registry, ComponentId id, PoolFactory factory) {
            registry.ensurePoolById(id, factory);
        }
        static bool hasPoolById(const Registry& registry, ComponentId id) {
            return registry.hasPoolById(id);
        }
        static bool hasComponentsById(const Registry& registry, Entity entity, std::initializer_list<ComponentId> ids) {
            return registry.hasComponentsById(entity, ids);
        }
        static void* getComponentById(Registry& registry, Entity entity, ComponentId id) {
            return registry.getComponentById(entity, id);
        }
        static void* addDefaultComponentById(Registry& registry, Entity entity, ComponentId id) {
            return registry.addDefaultComponentById(entity, id);
        }
        static void removeComponentById(Registry& registry, Entity entity, ComponentId id) {
            registry.removeComponentById(entity, id);
        }
        static std::vector<ComponentId> componentIdsOf(const Registry& registry, Entity entity) {
            return registry.componentIdsOf(entity);
        }
    };
}
