#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

#include "ComponentStorage.hpp"

namespace ECS {
    /**
     * @brief Type-erased interface shared by all runtime component pools.
     *
     * The interface intentionally exposes only operations needed by the
     * registry to destroy an entity. Typed access remains in ComponentPool<T>,
     * so the gameplay path does not require a base component class.
     */
    class IComponentPool {
        public:
            virtual ~IComponentPool() = default;

            /** @brief Checks whether the pool contains an entity identity. */
            [[nodiscard]] virtual bool has(std::size_t entityId) const noexcept = 0;

            /** @brief Removes an entity's component if it exists. */
            virtual void erase(std::size_t entityId) noexcept = 0;
    };

    /**
    * @brief Type-erased adapter around a typed component storage.
     * @tparam Component Component type stored by this pool.
     */
    template<typename Component>
    class ComponentPool final : public IComponentPool {
        public:
            /** @brief Returns the concrete component storage. */
            [[nodiscard]] ComponentStorage<Component>& storage() noexcept { return _storage; }

            /** @copydoc storage() */
            [[nodiscard]] const ComponentStorage<Component>& storage() const noexcept {
                return _storage;
            }

            /** @copydoc IComponentPool::has */
            [[nodiscard]] bool has(std::size_t entityId) const noexcept override {
                return _storage.has(entityId);
            }

            /** @copydoc IComponentPool::erase */
            void erase(std::size_t entityId) noexcept override {
                if (_onRemove && _storage.has(entityId)) {
                    try {
                        _onRemove(_storage.get(entityId));
                    } catch (...) {}
                }
                _storage.erase(entityId);
            }

            /**
             * @brief Sets a callback invoked with the exact instance about to
             *        be erased, right before the swap-and-pop destroys it.
             *
             * Fires from both removeComponent and killEntity, since both go
             * through erase(). A throwing callback is caught and discarded so
             * it can never leave erase(), which must stay noexcept.
             */
            void setOnRemove(std::function<void(Component&)> callback) {
                _onRemove = std::move(callback);
            }

        private:
            ComponentStorage<Component> _storage;
            std::function<void(Component&)> _onRemove;
    };

    /** @brief Creates an empty pool for one component type, without naming it. */
    using PoolFactory = std::unique_ptr<IComponentPool> (*)();

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
