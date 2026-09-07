#pragma once

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

namespace ECS {
    /**
     * @brief Dense component storage with sparse entity-to-component lookup.
     * @tparam Component Component value stored by this pool.
     * @tparam Allocator Allocator used by the dense component vector.
     *
     * Components are kept densely in `_packed`, which makes iteration cache
     * friendly. `_sparse` maps an entity identity to its packed index. Erasing
     * uses swap-and-pop, so component order is not stable after removal.
     */
    template<typename Component, typename Allocator = std::allocator<Component>>
    class SparseSet {
        public:
            using value_type = Component;
            using reference = value_type&;
            using const_reference = const value_type&;
            using reference_type = reference;
            using const_reference_type = const_reference;
            using size_type = std::size_t;

            /** @brief Sentinel used for entities without a component. */
            static constexpr size_type npos = static_cast<size_type>(-1);

            /** @brief Returns the number of stored components. */
            [[nodiscard]] size_type size() const noexcept { return _packed.size(); }

            /** @brief Reserves dense and sparse storage for a known workload. */
            void reserve(size_type componentCapacity, size_type entityCapacity = 0) {
                _packed.reserve(componentCapacity);
                _packedEntities.reserve(componentCapacity);
                if (entityCapacity > _sparse.capacity()) {
                    _sparse.reserve(entityCapacity);
                }
            }

            /** @brief Provides dense iteration over all stored components. */
            auto begin() noexcept { return _packed.begin(); }
            auto end() noexcept { return _packed.end(); }
            auto begin() const noexcept { return _packed.begin(); }
            auto end() const noexcept { return _packed.end(); }

            /**
             * @brief Checks whether an entity owns a component in this pool.
             * @param entity_id Numeric entity identity.
             */
            [[nodiscard]] bool has(size_type entity_id) const noexcept {
                return entity_id < _sparse.size() && _sparse[entity_id] != npos;
            }

            /**
             * @brief Inserts or replaces a component for an entity.
             * @param entity_id Numeric entity identity.
             * @param component Value to copy into the pool.
             * @return Reference to the inserted or replaced value.
             */
            reference insertAt(size_type entityId, const Component& component) {
                ensureSparseSize(entityId);
                if (has(entityId)) {
                    return _packed[_sparse[entityId]] = component;
                }
                append(entityId, component);
                return _packed.back();
            }

            /**
             * @brief Inserts or replaces a component for an entity by move.
             * @param entity_id Numeric entity identity.
             * @param component Value moved into the pool.
             * @return Reference to the inserted or replaced value.
             */
            reference insertAt(size_type entityId, Component&& component) {
                ensureSparseSize(entityId);
                if (has(entityId)) {
                    return _packed[_sparse[entityId]] = std::move(component);
                }
                append(entityId, std::move(component));
                return _packed.back();
            }

            /**
             * @brief Constructs or replaces a component in place.
             * @param entity_id Numeric entity identity.
             * @param params Arguments forwarded to Component's constructor.
             * @return Reference to the inserted or replaced value.
             */
            template<typename... Params>
            reference emplaceAt(size_type entityId, Params&&... params) {
                ensureSparseSize(entityId);
                if (has(entityId)) {
                    return _packed[_sparse[entityId]] =
                        Component(std::forward<Params>(params)...);
                }
                _packed.emplace_back(std::forward<Params>(params)...);
                _packedEntities.push_back(entityId);
                _sparse[entityId] = _packed.size() - 1;
                return _packed.back();
            }

            /**
             * @brief Removes an entity's component using swap-and-pop.
             * @param entity_id Numeric entity identity.
             *
             * Removing a middle element moves the final packed component into
             * its slot. Iteration order and references to moved values are not
             * stable across this operation. Calling erase for a missing entity
             * is harmless.
             */
            void erase(size_type entityId) {
                if (!has(entityId)) {
                    return;
                }

                const size_type packedIndex = _sparse[entityId];
                const size_type lastIndex = _packed.size() - 1;
                if (packedIndex != lastIndex) {
                    std::swap(_packed[packedIndex], _packed[lastIndex]);
                    std::swap(_packedEntities[packedIndex], _packedEntities[lastIndex]);
                    _sparse[_packedEntities[packedIndex]] = packedIndex;
                }

                _packed.pop_back();
                _packedEntities.pop_back();
                _sparse[entityId] = npos;
            }

            /**
             * @brief Accesses a component by entity identity.
             * @throws std::out_of_range if the entity has no component.
             */
            reference get(size_type entityId) {
                if (!has(entityId)) {
                    throw std::out_of_range("ECS::SparseSet::get: component not found");
                }
                return _packed[_sparse[entityId]];
            }

            /** @copydoc get(size_type) */
            const_reference get(size_type entityId) const {
                if (!has(entityId)) {
                    throw std::out_of_range("ECS::SparseSet::get: component not found");
                }
                return _packed[_sparse[entityId]];
            }

            /** @brief Returns the entity identity stored at a packed position. */
            [[nodiscard]] size_type entityAt(size_type packedIndex) const {
                return _packedEntities.at(packedIndex);
            }

            /** @brief Provides unchecked dense component access for query code. */
            reference operator[](size_type packedIndex) noexcept { return _packed[packedIndex]; }
            /** @copydoc operator[](size_type) */
            const_reference operator[](size_type packedIndex) const noexcept {
                return _packed[packedIndex];
            }

        private:
            void ensureSparseSize(size_type entityId) {
                if (entityId >= _sparse.size()) {
                    _sparse.resize(entityId + 1, npos);
                }
            }

            template<typename Value>
            void append(size_type entityId, Value&& value) {
                _packed.push_back(std::forward<Value>(value));
                _packedEntities.push_back(entityId);
                _sparse[entityId] = _packed.size() - 1;
            }

            std::vector<Component, Allocator> _packed;
                std::vector<size_type> _packedEntities;
            std::vector<size_type> _sparse;
    };

    template<typename Component, typename Allocator>
    constexpr typename SparseSet<Component, Allocator>::size_type
        SparseSet<Component, Allocator>::npos;

}