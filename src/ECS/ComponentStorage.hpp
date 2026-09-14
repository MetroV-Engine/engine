#pragma once

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

namespace ECS {
    /**
     * @brief Component storage with dense iteration and entity lookup.
     * @tparam Component Component value stored by this storage.
     * @tparam Allocator Allocator used by the dense component vector.
     *
     * The representation is private so it can be replaced later without
     * changing the registry or view APIs.
     */
    template<typename Component, typename Allocator = std::allocator<Component>>
    class ComponentStorage {
        public:
            using value_type = Component;
            using reference = value_type&;
            using const_reference = const value_type&;
            using reference_type = reference;
            using const_reference_type = const_reference;
            using size_type = std::size_t;

            /** @brief Sentinel used by the entity-to-packed-index lookup. */
            static constexpr size_type npos = static_cast<size_type>(-1);

            /** @brief Returns the number of components currently stored. */
            [[nodiscard]] size_type size() const noexcept { return _packed.size(); }

            /**
             * @brief Reserves capacity for components and optional entity IDs.
             * @param componentCapacity Expected number of stored components.
             * @param entityCapacity Expected highest entity lookup capacity.
             */
            void reserve(size_type componentCapacity, size_type entityCapacity = 0) {
                _packed.reserve(componentCapacity);
                _packedEntities.reserve(componentCapacity);
                if (entityCapacity > _sparse.capacity()) {
                    _sparse.reserve(entityCapacity);
                }
            }

            /** @brief Begins dense iteration over component values. */
            auto begin() noexcept { return _packed.begin(); }
            /** @brief Ends dense iteration over component values. */
            auto end() noexcept { return _packed.end(); }
            /** @copydoc begin() */
            auto begin() const noexcept { return _packed.begin(); }
            /** @copydoc end() */
            auto end() const noexcept { return _packed.end(); }

            /** @brief Checks whether an entity has a component in this storage. */
            [[nodiscard]] bool has(size_type entityId) const noexcept {
                return entityId < _sparse.size() && _sparse[entityId] != npos;
            }

            /** @brief Inserts or replaces a component by copy. */
            reference insertAt(size_type entityId, const Component& component) {
                ensureSparseSize(entityId);
                if (has(entityId)) {
                    return _packed[_sparse[entityId]] = component;
                }
                append(entityId, component);
                return _packed.back();
            }

            /** @brief Inserts or replaces a component by move. */
            reference insertAt(size_type entityId, Component&& component) {
                ensureSparseSize(entityId);
                if (has(entityId)) {
                    return _packed[_sparse[entityId]] = std::move(component);
                }
                append(entityId, std::move(component));
                return _packed.back();
            }

            /** @brief Constructs or replaces a component in place. */
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
             * @brief Removes a component using swap-and-pop.
             *
             * Packed order is not stable after removal. Structural changes
             * should not be made while a view is iterating this storage.
             */
            void erase(size_type entityId) noexcept {
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
             * @brief Returns a mutable component by entity ID.
             * @throws std::out_of_range when the entity has no component.
             */
            reference get(size_type entityId) {
                if (!has(entityId)) {
                    throw std::out_of_range("ECS::ComponentStorage::get: component not found");
                }
                return _packed[_sparse[entityId]];
            }

            /** @copydoc get(size_type) */
            const_reference get(size_type entityId) const {
                if (!has(entityId)) {
                    throw std::out_of_range("ECS::ComponentStorage::get: component not found");
                }
                return _packed[_sparse[entityId]];
            }

            /** @brief Returns the entity ID at a dense component position. */
            [[nodiscard]] size_type entityAt(size_type packedIndex) const {
                return _packedEntities.at(packedIndex);
            }

            /** @brief Returns unchecked mutable access by dense position. */
            reference operator[](size_type packedIndex) noexcept { return _packed[packedIndex]; }
            /** @brief Returns unchecked read-only access by dense position. */
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
    constexpr typename ComponentStorage<Component, Allocator>::size_type
        ComponentStorage<Component, Allocator>::npos;
}
