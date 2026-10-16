#pragma once

#include <cstddef>
#include <functional>
#include <initializer_list>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

#include "ComponentId.hpp"
#include "ComponentOps.hpp"
#include "ComponentPool.hpp"
#include "ComponentStorage.hpp"
#include "SignatureTable.hpp"

namespace ECS {
    /**
     * @brief Every component pool of a world, and which components each entity owns.
     *
     * Holds one pool per component type, indexed by componentId(), and the
     * SignatureTable that mirrors them: each entity's row has one bit per
     * component it owns. Adding or removing a component always goes through
     * this store, so the pools and the rows can't disagree.
     *
     * Works on entity indices and knows nothing about liveness: the Registry
     * checks that an entity is alive before calling it.
     */
    class ComponentStore {
        public:
            // --- Typed access -------------------------------------------------

            /** @brief Registers Component if needed and returns its storage. */
            template<typename Component>
            ComponentStorage<Component>& ensure() {
                const ComponentId id = componentId<Component>();
                ensurePool(id, &makePool<Component>);
                return typedPool<Component>(id).storage();
            }

            /**
             * @brief Returns the storage of a registered Component.
             * @throws std::out_of_range when Component has not been registered.
             */
            template<typename Component>
            [[nodiscard]] ComponentStorage<Component>& storage() {
                const ComponentId id = componentId<Component>();
                requireRegistered(id);
                return typedPool<Component>(id).storage();
            }

            /** @copydoc storage() */
            template<typename Component>
            [[nodiscard]] const ComponentStorage<Component>& storage() const {
                const ComponentId id = componentId<Component>();
                requireRegistered(id);
                return typedPool<Component>(id).storage();
            }

            /**
             * @brief Returns the storage of Component, or an empty one when it
             *        is not registered (without registering it).
             */
            template<typename Component>
            [[nodiscard]] const ComponentStorage<Component>& storageOrEmpty() const noexcept {
                static const ComponentStorage<Component> empty;
                const ComponentStorage<Component>* found = find<Component>();
                return found ? *found : empty;
            }

            /** @brief Returns the storage of Component when registered, otherwise nullptr. */
            template<typename Component>
            [[nodiscard]] const ComponentStorage<Component>* find() const noexcept {
                const ComponentId id = componentId<Component>();
                return hasPool(id) ? &typedPool<Component>(id).storage() : nullptr;
            }

            /**
             * @brief Adds or replaces an entity's component, registering Component if needed.
             * @return Reference to the stored component.
             */
            template<typename Component>
            Component& insert(std::size_t entity, Component value) {
                Component& stored = ensure<Component>().insertAt(entity, std::move(value));
                markOwned(entity, componentId<Component>());
                return stored;
            }

            /**
             * @brief Sets the callback fired with each Component right before it
             *        is erased (remove or removeEntity). Registers Component if needed.
             */
            template<typename Component>
            void onRemove(std::function<void(Component&)> callback) {
                ensure<Component>();
                typedPool<Component>(componentId<Component>()).setOnRemove(std::move(callback));
            }

            // --- ComponentId-based access -------------------------------------

            /**
             * @brief Creates the pool for id with factory, unless it already exists.
             * @param factory Must create the pool of the type whose componentId() is id.
             */
            void ensurePool(ComponentId id, PoolFactory factory) {
                if (id >= _pools.size()) {
                    _pools.resize(static_cast<std::size_t>(id) + 1);
                }
                if (!_pools[id]) {
                    _signatures.ensureComponent(id);
                    _pools[id] = factory();
                }
            }

            /** @throws std::out_of_range when there is no pool for id. */
            void requireRegistered(ComponentId id) const {
                if (!hasPool(id)) {
                    throw std::out_of_range("ECS::ComponentStore: component not registered");
                }
            }

            /** @brief Checks whether a pool exists for id. */
            [[nodiscard]] bool hasPool(ComponentId id) const noexcept {
                return id < _pools.size() && _pools[id] != nullptr;
            }

            /**
             * @brief Checks whether an entity owns every listed component.
             * @return True for an empty list. A type without a pool is owned by no entity.
             */
            [[nodiscard]] bool has(std::size_t entity, std::initializer_list<ComponentId> ids) const noexcept {
                for (const ComponentId id : ids) {
                    if (!_signatures.test(entity, id)) {
                        return false;
                    }
                }
                return true;
            }

            /**
             * @brief Address of an entity's component, or nullptr when absent.
             * @param ops Operations of the type whose componentId() is id.
             */
            [[nodiscard]] void* get(std::size_t entity, ComponentId id, const ComponentOps& ops) {
                return hasPool(id) ? ops.get(*_pools[id], entity) : nullptr;
            }

            /**
             * @brief Moves *value into the entity's slot (insert or replace).
             * @param ops Operations of the type whose componentId() is id.
             * @return Address of the stored component.
             * @throws std::out_of_range when there is no pool for id.
             */
            void* insertMoved(std::size_t entity, ComponentId id, void* value, const ComponentOps& ops) {
                requireRegistered(id);
                void* stored = ops.insertMoved(*_pools[id], entity, value);
                markOwned(entity, id);
                return stored;
            }

            /** @brief Removes an entity's component; does nothing when it has none. */
            void remove(std::size_t entity, ComponentId id) {
                if (!_signatures.test(entity, id)) {
                    return;
                }
                _pools[id]->erase(entity);
                _signatures.reset(entity, id);
            }

            /** @brief ComponentIds of every component an entity owns, ascending. */
            [[nodiscard]] std::vector<ComponentId> idsOf(std::size_t entity) const {
                std::vector<ComponentId> ids;
                for (ComponentId id = 0; id < _pools.size(); ++id) {
                    if (_signatures.test(entity, id)) {
                        ids.push_back(id);
                    }
                }
                return ids;
            }

            // --- Entity lifecycle -------------------------------------------------

            /** @brief Gives a new (or recycled) entity an empty row. */
            void addEntity(std::size_t entity) {
                _signatures.ensureEntity(entity);
                _signatures.clear(entity);
            }

            /** @brief Erases every component of an entity (firing onRemove) and empties its row. */
            void removeEntity(std::size_t entity) {
                for (const auto& pool : _pools) {
                    if (pool) {
                        pool->erase(entity);
                    }
                }
                if (_signatures.hasEntity(entity)) {
                    _signatures.clear(entity);
                }
            }

        private:
            void markOwned(std::size_t entity, ComponentId id) {
                _signatures.ensureEntity(entity);
                _signatures.set(entity, id);
            }

            template<typename Component>
            ComponentPool<Component>& typedPool(ComponentId id) {
                return *static_cast<ComponentPool<Component>*>(_pools[id].get());
            }

            template<typename Component>
            const ComponentPool<Component>& typedPool(ComponentId id) const {
                return *static_cast<const ComponentPool<Component>*>(_pools[id].get());
            }

            std::vector<std::unique_ptr<IComponentPool>> _pools;
            SignatureTable _signatures;
    };
}
