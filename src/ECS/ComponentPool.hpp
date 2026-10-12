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
     * Exposes what the registry needs without knowing the component type:
     * destroying an entity, and the ComponentId-based access used by tools
     * (editor, scripts, scenes) through the reflection layer. Typed access
     * remains in ComponentPool<T>, so the gameplay path does not require a
     * base component class.
     */
    class IComponentPool {
        public:
            virtual ~IComponentPool() = default;

            /** @brief Checks whether the pool contains an entity identity. */
            [[nodiscard]] virtual bool has(std::size_t entityId) const noexcept = 0;

            /** @brief Removes an entity's component if it exists. */
            virtual void erase(std::size_t entityId) noexcept = 0;

            /** @brief Address of an entity's component, or nullptr when absent. */
            [[nodiscard]] virtual void* getRaw(std::size_t entityId) noexcept = 0;

            /**
             * @brief Creates a default-constructed component outside the pool.
             * @return nullptr when the component type has no default constructor.
             */
            [[nodiscard]] virtual std::shared_ptr<void> makeDefault() const = 0;

            /**
             * @brief Moves *value (a component of this pool's type) into the
             *        entity's slot, inserting or replacing.
             * @return Address of the stored component. (Components are always
             *         movable: the storage's swap-and-pop erase requires it.)
             */
            virtual void* insertMoved(std::size_t entityId, void* value) = 0;
    };

    /** @brief Creates an empty pool for one component type, without naming it. */
    using PoolFactory = std::unique_ptr<IComponentPool> (*)();

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

            /** @copydoc IComponentPool::getRaw */
            [[nodiscard]] void* getRaw(std::size_t entityId) noexcept override {
                return _storage.has(entityId) ? &_storage.get(entityId) : nullptr;
            }

            /** @copydoc IComponentPool::makeDefault */
            [[nodiscard]] std::shared_ptr<void> makeDefault() const override {
                if constexpr (std::is_default_constructible_v<Component>) {
                    return std::make_shared<Component>();
                } else {
                    return nullptr;
                }
            }

            /** @copydoc IComponentPool::insertMoved */
            void* insertMoved(std::size_t entityId, void* value) override {
                return &_storage.insertAt(entityId, std::move(*static_cast<Component*>(value)));
            }

            /** @brief PoolFactory creating a ComponentPool<Component>. */
            [[nodiscard]] static std::unique_ptr<IComponentPool> create() {
                return std::make_unique<ComponentPool<Component>>();
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
}