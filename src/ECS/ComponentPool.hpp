#pragma once

#include <cstddef>
#include <memory>

#include "SparseSet.hpp"

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
     * @brief Type-erased adapter around a typed SparseSet.
     * @tparam Component Component type stored by this pool.
     */
    template<typename Component>
    class ComponentPool final : public IComponentPool {
        public:
            /** @brief Returns the concrete component storage. */
            [[nodiscard]] SparseSet<Component>& storage() noexcept { return _storage; }

            /** @copydoc storage() */
            [[nodiscard]] const SparseSet<Component>& storage() const noexcept {
                return _storage;
            }

            /** @copydoc IComponentPool::has */
            [[nodiscard]] bool has(std::size_t entityId) const noexcept override {
                return _storage.has(entityId);
            }

            /** @copydoc IComponentPool::erase */
            void erase(std::size_t entityId) noexcept override {
                _storage.erase(entityId);
            }

        private:
            SparseSet<Component> _storage;
    };
}