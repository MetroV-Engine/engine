#pragma once

#include <cstddef>
#include <functional>
#include <memory>

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
}