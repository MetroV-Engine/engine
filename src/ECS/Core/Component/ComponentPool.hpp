#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <utility>

#include "ComponentStorage.hpp"

namespace ECS {
    /**
     * @brief Type-erased interface shared by all runtime component pools.
     *
     * Exposes only what the ComponentStore needs without knowing the type:
     * erasing an entity's component when the entity is killed. Membership
     * is answered by the SignatureTable, typed access by ComponentPool<T>.
     */
    class IComponentPool {
        public:
            virtual ~IComponentPool() = default;

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

    /** @brief The PoolFactory of Component: creates an empty ComponentPool<Component>. */
    template<typename Component>
    [[nodiscard]] std::unique_ptr<IComponentPool> makePool() {
        return std::make_unique<ComponentPool<Component>>();
    }
}
