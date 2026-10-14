#pragma once

#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>

#include "ComponentPool.hpp"

namespace ECS {
    /**
     * @brief Type-specific operations on a component type, for code that
     *        only knows its ComponentId (the reflection layer).
     *
     * These are plain function pointers rather than virtual functions of
     * IComponentPool on purpose: a virtual function is compiled for every
     * component type, whereas of<Component>() is only compiled for the types
     * that call it (the reflected ones, from the component catalog). Pools of
     * other component types cost nothing more than before.
     */
    struct ComponentOps {
        /** @brief Creates an empty ComponentPool of the type. */
        PoolFactory makePool = nullptr;
        /** @brief Address of an entity's component in pool, or nullptr when absent. */
        void* (*get)(IComponentPool& pool, std::size_t entityId) noexcept = nullptr;
        /** @brief Heap-allocates a default-constructed component; nullptr if not default-constructible. */
        void* (*newDefault)() = nullptr;
        /** @brief Frees a component allocated by newDefault. */
        void (*deleteValue)(void* value) noexcept = nullptr;
        /** @brief Moves *value into the entity's slot of pool (insert or replace); returns its address. */
        void* (*insertMoved)(IComponentPool& pool, std::size_t entityId, void* value) = nullptr;

        /** @brief The operations of Component; pool arguments must be ComponentPool<Component>. */
        template<typename Component>
        [[nodiscard]] static ComponentOps of() noexcept {
            ComponentOps ops;
            ops.makePool = []() -> std::unique_ptr<IComponentPool> {
                return std::make_unique<ComponentPool<Component>>();
            };
            ops.get = [](IComponentPool& pool, std::size_t entityId) noexcept -> void* {
                auto& storage = static_cast<ComponentPool<Component>&>(pool).storage();
                return storage.has(entityId) ? &storage.get(entityId) : nullptr;
            };
            ops.newDefault = []() -> void* {
                if constexpr (std::is_default_constructible_v<Component>) {
                    return new Component();
                } else {
                    return nullptr;
                }
            };
            ops.deleteValue = [](void* value) noexcept {
                delete static_cast<Component*>(value);
            };
            ops.insertMoved = [](IComponentPool& pool, std::size_t entityId, void* value) -> void* {
                auto& storage = static_cast<ComponentPool<Component>&>(pool).storage();
                return &storage.insertAt(entityId, std::move(*static_cast<Component*>(value)));
            };
            return ops;
        }
    };
}
