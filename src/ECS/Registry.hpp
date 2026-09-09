#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
#include <unordered_map>

#include "ComponentId.hpp"
#include "ComponentStorage.hpp"
#include "ComponentPool.hpp"
#include "EntityManager.hpp"
#include "View.hpp"

namespace ECS {
    /**
     * @brief Header-only ECS registry for entities, component pools and systems.
     *
    * Registry keeps one component storage per component type. Component IDs are
     * allocated lazily and index the pool table, while EntityManager owns the
     * entity lifecycle. Systems receive typed pools and therefore avoid runtime
     * type lookup during their iteration.
     */
    class Registry {
        public:
            Registry() = default;
            Registry(const Registry&) = delete;
            Registry& operator=(const Registry&) = delete;

            /**
             * @brief Creates a named or unnamed entity.
             * @param name Optional display name stored by the registry.
             * @return Newly created entity handle.
             */
            Entity spawnEntity(const std::string& name = {}) {
                _entitiesDirty = true;
                const Entity entity = _entities.create();
                if (!name.empty()) {
                    _entityNames[entity.value()] = name;
                }
                return entity;
            }

            /**
             * @brief Reconstructs an entity handle from a numeric identity.
             * @param idx Entity identity.
             * @return Handle containing idx.
             *
             * This compatibility helper does not make an identity live.
             */
            [[nodiscard]] Entity entityFromIndex(std::size_t idx) const noexcept {
                return _entities.entityFromIndex(idx);
            }

            /**
             * @brief Destroys an entity and removes it from every component pool.
             * @param entity Entity to destroy.
             * @throws std::invalid_argument if entity is not alive.
             */
            void killEntity(Entity entity) {
                _entities.destroy(entity);
                for (const auto& pool : _pools) {
                    if (pool) {
                        pool->erase(entity.value());
                    }
                }
                _entityNames.erase(entity.value());
                _entitiesDirty = true;
            }

            /**
             * @brief Registers a component pool if absent and returns its storage.
             * @tparam Component Component type to register.
             * @return Typed dense storage for Component.
             */
            template<typename Component>
            ComponentStorage<Component>& registerComponent() {
                const ComponentId id = componentId<Component>();
                ensurePoolSlot(id);
                if (!_pools[id]) {
                    _pools[id] = std::make_unique<ComponentPool<Component>>();
                }
                return typedPool<Component>(id).storage();
            }

            /**
             * @brief Returns an already registered component pool.
             * @throws std::out_of_range when Component has not been registered.
             */
            template<typename Component>
            ComponentStorage<Component>& getComponents() {
                const ComponentId id = componentId<Component>();
                if (id >= _pools.size() || !_pools[id]) {
                    throw std::out_of_range("ECS::Registry::getComponents: component not registered");
                }
                return typedPool<Component>(id).storage();
            }

            /** @copydoc getComponents() */
            template<typename Component>
            const ComponentStorage<Component>& getComponents() const {
                const ComponentId id = componentId<Component>();
                if (id >= _pools.size() || !_pools[id]) {
                    throw std::out_of_range("ECS::Registry::getComponents: component not registered");
                }
                return typedPool<Component>(id).storage();
            }

            /**
             * @brief Returns a component for an entity.
             * @throws std::out_of_range if the pool or component is absent.
             */
            template<typename Component>
            Component& getComponent(Entity entity) {
                ensureEntityAlive(entity);
                return getComponents<Component>().get(entity.value());
            }

            /** @copydoc getComponent(Entity) */
            template<typename Component>
            const Component& getComponent(Entity entity) const {
                ensureEntityAlive(entity);
                return getComponents<Component>().get(entity.value());
            }

            /**
             * @brief Checks whether an entity owns a component.
             * @param entity Entity identity to inspect.
             * @return True when the component pool exists and contains entity.
             */
            template<typename Component>
            [[nodiscard]] bool hasComponent(Entity entity) const noexcept {
                if (!_entities.isAlive(entity)) {
                    return false;
                }
                const ComponentId id = componentId<Component>();
                if (id >= _pools.size() || !_pools[id]) {
                    return false;
                }
                return typedPool<Component>(id).has(entity.value());
            }

            /**
             * @brief Adds or replaces a component using copy or move semantics.
             * @tparam Component Deduced component value type.
             * @return Reference to the stored component.
             */
            template<typename Component>
            std::decay_t<Component>& addComponent(Entity entity, Component&& component) {
                ensureEntityAlive(entity);
                using StoredComponent = std::decay_t<Component>;
                auto& pool = registerComponent<StoredComponent>();
                return pool.insertAt(entity.value(), std::forward<Component>(component));
            }

            /**
             * @brief Constructs or replaces a component for an entity.
             * @return Reference to the stored component.
             */
            template<typename Component, typename... Params>
            Component& emplaceComponent(Entity entity, Params&&... params) {
                ensureEntityAlive(entity);
                return registerComponent<Component>().emplaceAt(
                    entity.value(), std::forward<Params>(params)...);
            }

            /**
             * @brief Removes a component from an entity when its pool exists.
             * @param entity Entity whose component should be removed.
             */
            template<typename Component>
            void removeComponent(Entity entity) {
                ensureEntityAlive(entity);
                auto* pool = getIf<Component>();
                if (pool) {
                    pool->erase(entity.value());
                }
            }

            /**
             * @brief Returns a typed pool when registered, otherwise nullptr.
             */
            template<typename Component>
            ComponentStorage<Component>* getIf() noexcept {
                const ComponentId id = componentId<Component>();
                if (id >= _pools.size() || !_pools[id]) {
                    return nullptr;
                }
                return &typedPool<Component>(id).storage();
            }

            /** @brief Creates a query over entities with all requested components. */
            template<typename... Components>
            View<Components...> view() {
                return View<Components...>(_entities, getComponents<Components>()...);
            }

            /** @brief Creates a read-only query over entities with all requested components. */
            template<typename... Components>
            ReadOnlyView<Components...> view() const {
                return ReadOnlyView<Components...>(_entities, getComponents<Components>()...);
            }

            /**
             * @brief Registers a callable system receiving the registry facade.
             * @tparam Components Component types required by the system. Each
             *         is registered immediately so a `view<Components...>()`
             *         call inside the system body never throws for a type
             *         that no entity has received yet.
             * @tparam Function Callable accepting `(Registry&)`.
             */
            template<typename... Components, typename Function>
            void addSystem(Function&& function) {
                (registerComponent<Components>(), ...);
                using FunctionType = std::decay_t<Function>;
                _systems.emplace_back(
                    [callable = FunctionType(std::forward<Function>(function))](Registry& world) mutable {
                        callable(world);
                    });
            }

            /** @brief Executes registered systems in registration order. */
            void runSystems() {
                for (auto& system : _systems) {
                    system(*this);
                }
            }

            /**
             * @brief Returns all currently live entities for tooling or inspection.
             * @return A reference to a cache rebuilt only when entities changed
             *         since the last call. The reference is invalidated by any
             *         subsequent call that rebuilds the cache (spawn, kill, or
             *         a name change), so do not hold it across those calls.
             */
            [[nodiscard]] const std::vector<Entity>& getAllEntities() const {
                if (_entitiesDirty) {
                    _cachedEntities = _entities.getAll();
                    _entitiesDirty = false;
                }
                return _cachedEntities;
            }

            /** @brief Associates a display name with an entity. */
            void setEntityName(Entity entity, const std::string& name) {
                if (!_entities.isAlive(entity)) {
                    throw std::invalid_argument("ECS::Registry::setEntityName: entity is not alive");
                }
                _entityNames[entity.value()] = name;
                _entitiesDirty = true;
            }

            /** @brief Returns an entity's display name, or an empty string. */
            [[nodiscard]] std::string getEntityName(Entity entity) const {
                if (!_entities.isAlive(entity)) {
                    return {};
                }
                const auto it = _entityNames.find(entity.value());
                return it == _entityNames.end() ? std::string{} : it->second;
            }

        private:
            void ensureEntityAlive(Entity entity) const {
                if (!_entities.isAlive(entity)) {
                    throw std::invalid_argument("ECS::Registry: entity handle is not alive");
                }
            }

            void ensurePoolSlot(ComponentId id) {
                if (id >= _pools.size()) {
                    _pools.resize(static_cast<std::size_t>(id) + 1);
                }
            }

            template<typename Component>
            ComponentPool<Component>& typedPool(ComponentId id) {
                return *static_cast<ComponentPool<Component>*>(_pools[id].get());
            }

            template<typename Component>
            const ComponentPool<Component>& typedPool(ComponentId id) const {
                return *static_cast<const ComponentPool<Component>*>(_pools[id].get());
            }

            EntityManager _entities;
            std::vector<std::unique_ptr<IComponentPool>> _pools;
            std::vector<std::function<void(Registry&)>> _systems;
            std::unordered_map<std::size_t, std::string> _entityNames;
            mutable std::vector<Entity> _cachedEntities;
            mutable bool _entitiesDirty{true};
    };
}