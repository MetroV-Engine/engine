#pragma once

#include <initializer_list>
#include <vector>

#include "ECS/Core/Component/ComponentId.hpp"
#include "ECS/Core/Component/ComponentOps.hpp"
#include "ECS/Core/Component/ComponentPool.hpp"
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
        /** @brief getComponentById with Component's ComponentId and operations. */
        template<typename Component>
        static void* getComponentById(Registry& registry, Entity entity) {
            return registry.getComponentById(entity, componentId<Component>(), ComponentOps::of<Component>());
        }
        /** @brief addDefaultComponentById with Component's ComponentId and operations. */
        template<typename Component>
        static void* addDefaultComponentById(Registry& registry, Entity entity) {
            return registry.addDefaultComponentById(entity, componentId<Component>(), ComponentOps::of<Component>());
        }
        static void removeComponentById(Registry& registry, Entity entity, ComponentId id) {
            registry.removeComponentById(entity, id);
        }
        static std::vector<ComponentId> componentIdsOf(const Registry& registry, Entity entity) {
            return registry.componentIdsOf(entity);
        }
    };
}
