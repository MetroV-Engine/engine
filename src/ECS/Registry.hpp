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
#include "ResourceHolder.hpp"
#include "ResourceId.hpp"
#include "SignatureTable.hpp"
#include "View.hpp"

namespace ECS {
    /**
     * @brief Header-only ECS registry for entities, component pools and systems.
     *
     * Registry keeps one component storage per component type. Component IDs are
     * allocated lazily and index the pool table, while EntityManager owns the
     * entity lifecycle.
     *
     * The pools are the source of truth for which components an entity owns.
     * Each entity's row in the SignatureTable mirrors them and is only
     * updated here, which is why storages are handed out read-only: adding
     * or removing a component must go through the Registry. The table widens
     * when a component type is registered, so there is no limit on how many
     * types a registry can hold.
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
                if (_deferring) {
                    return spawnEntityDeferred(name);
                }
                return spawnEntityImmediate(name);
            }

            /**
             * @brief Returns the WorldId stamped on every handle this registry issues.
             *
             * Handles issued by another registry carry a different WorldId and
             * are rejected as not alive here.
             */
            [[nodiscard]] WorldId worldId() const noexcept {
                return _entities.worldId();
            }

            /**
             * @brief Destroys an entity and removes it from every component pool.
             * @param entity Entity to destroy.
             * @throws std::invalid_argument if entity is not alive.
             */
            void killEntity(Entity entity) {
                if (_deferring) {
                    killEntityDeferred(entity);
                    return;
                }
                killEntityImmediate(entity);
            }

            /**
             * @brief Registers a component pool if absent and returns its storage.
             * @tparam Component Component type to register.
             * @return Read-only typed dense storage for Component.
             */
            template<typename Component>
            const ComponentStorage<Component>& registerComponent() {
                return ensureStorage<Component>();
            }

            /**
             * @brief Returns an already registered component storage, read-only.
             * @throws std::out_of_range when Component has not been registered.
             */
            template<typename Component>
            const ComponentStorage<Component>& getComponents() const {
                const ComponentId id = componentId<Component>();
                requireRegistered(id);
                return typedPool<Component>(id).storage();
            }

            /**
             * @brief Returns a component for an entity.
             * @throws std::out_of_range if the pool or component is absent.
             */
            template<typename Component>
            Component& getComponent(Entity entity) {
                ensureEntityAlive(entity);
                return mutableStorage<Component>().get(entity.value());
            }

            /** @copydoc getComponent(Entity) */
            template<typename Component>
            const Component& getComponent(Entity entity) const {
                ensureEntityAlive(entity);
                return getComponents<Component>().get(entity.value());
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
                StoredComponent value(std::forward<Component>(component));
                if (_deferring) {
                    return addComponentDeferred<StoredComponent>(entity, std::move(value));
                }
                return addComponentImmediate<StoredComponent>(entity, std::move(value));
            }

            /**
             * @brief Constructs or replaces a component for an entity.
             * @return Reference to the stored component.
             */
            template<typename Component, typename... Params>
            Component& emplaceComponent(Entity entity, Params&&... params) {
                ensureEntityAlive(entity);
                Component value(std::forward<Params>(params)...);
                if (_deferring) {
                    return emplaceComponentDeferred<Component>(entity, std::move(value));
                }
                return emplaceComponentImmediate<Component>(entity, std::move(value));
            }

            /**
             * @brief Removes a component from an entity when its pool exists.
             * @param entity Entity whose component should be removed.
             */
            template<typename Component>
            void removeComponent(Entity entity) {
                ensureEntityAlive(entity);
                if (_deferring) {
                    removeComponentDeferred<Component>(entity);
                    return;
                }
                removeComponentImmediate<Component>(entity);
            }

            /**
             * @brief Checks whether an entity owns every requested component type.
             * @tparam Components Component types the entity must all own.
             * @return True when the entity is alive and its signature row has
             *         every requested type's bit set. The row mirrors the
             *         pools; a type never registered here has no bit set for
             *         any entity.
             */
            template<typename... Components>
            [[nodiscard]] bool hasComponents(Entity entity) const {
                if (!_entities.isAlive(entity)) {
                    return false;
                }
                return (_signatures.test(entity.value(), componentId<Components>()) && ...);
            }

            /**
             * @brief Returns a read-only typed storage when registered, otherwise nullptr.
             */
            template<typename Component>
            const ComponentStorage<Component>* getIf() const noexcept {
                const ComponentId id = componentId<Component>();
                if (id >= _pools.size() || !_pools[id]) {
                    return nullptr;
                }
                return &typedPool<Component>(id).storage();
            }

            /**
             * @brief Creates a query over entities with all requested components.
             *
             * Any requested Component not yet registered gets an empty pool
             * registered for it, so a system can query a type before any
             * entity has received it and simply see no matches, instead of
             * this throwing.
             */
            template<typename... Components>
            View<Components...> view() {
                (ensureStorage<Components>(), ...);
                return View<Components...>(_entities, mutableStorage<Components>()...);
            }

            /**
             * @brief Creates a read-only query over entities with all requested components.
             * @throws std::out_of_range when a requested Component has never
             *         been registered. Unlike the mutable overload, this
             *         can't register an empty pool as a side effect: it is
             *         const, and a read-only query shouldn't create storage.
             */
            template<typename... Components>
            ReadOnlyView<Components...> view() const {
                return ReadOnlyView<Components...>(_entities, getComponents<Components>()...);
            }

            /**
             * @brief Runs callable under deferred-mutation mode.
             *
             * Structural mutations (spawnEntity, killEntity, addComponent,
             * emplaceComponent, removeComponent) made from within callable are
             * deferred: they queue instead of touching component storage
             * immediately, so a View being iterated can't be reordered out
             * from under it. Queued commands apply, in the order they were
             * recorded, once callable returns or throws.
             *
             * Deferred mode is turned off and queued commands are flushed
             * even if callable throws, so a failing caller (e.g. a system
             * that throws mid-update) can't leave the registry stuck in
             * deferred mode.
             */
            template<typename Callable>
            void runDeferred(Callable&& callable) {
                _deferring = true;
                struct DeferGuard {
                    Registry& registry;
                    ~DeferGuard() {
                        registry._deferring = false;
                        registry.flushCommands();
                    }
                } guard{*this};
                callable();
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

            /**
             * @brief Constructs or replaces the Resource-typed value held by this registry.
             * @tparam Resource Resource type, distinct from any component's ID space.
             * @param params Constructor arguments forwarded to Resource.
             * @return Reference to the stored resource.
             *
             * Resources are not structural entity state, so this always applies
             * immediately, even while a system is running under runDeferred.
             */
            template<typename Resource, typename... Params>
            Resource& setResource(Params&&... params) {
                const ResourceId id = resourceId<Resource>();
                ensureResourceSlot(id);
                auto holder = std::make_unique<ResourceHolder<Resource>>(std::forward<Params>(params)...);
                Resource& ref = holder->value();
                _resources[id] = std::move(holder);
                return ref;
            }

            /**
             * @brief Returns a previously set resource.
             * @throws std::out_of_range when Resource has never been set.
             */
            template<typename Resource>
            [[nodiscard]] Resource& getResource() {
                const ResourceId id = resourceId<Resource>();
                requireResourceSet(id);
                return typedResource<Resource>(id).value();
            }

            /** @copydoc getResource() */
            template<typename Resource>
            [[nodiscard]] const Resource& getResource() const {
                const ResourceId id = resourceId<Resource>();
                requireResourceSet(id);
                return typedResource<Resource>(id).value();
            }

            /** @brief Checks whether a resource of this type has been set. */
            template<typename Resource>
            [[nodiscard]] bool hasResource() const noexcept {
                const ResourceId id = resourceId<Resource>();
                return id < _resources.size() && _resources[id] != nullptr;
            }

            /**
             * @brief Registers a callback fired with the exact instance right
             *        before a Component is erased, from removeComponent or
             *        killEntity alike.
             * @tparam Component Component type whose pool receives the hook.
             * @param callback Invoked once per erased instance; exceptions it
             *        throws are caught and discarded, never propagated.
             */
            template<typename Component>
            void onRemove(std::function<void(Component&)> callback) {
                ensureStorage<Component>();
                typedPool<Component>(componentId<Component>()).setOnRemove(std::move(callback));
            }

        private:
            Entity spawnEntityImmediate(const std::string& name) {
                _entitiesDirty = true;
                const Entity entity = _entities.create();
                _signatures.ensureEntity(entity.value());
                _signatures.clear(entity.value());
                if (!name.empty()) {
                    _entityNames[entity.value()] = name;
                }
                return entity;
            }

            Entity spawnEntityDeferred(const std::string& name) {
                const Entity entity = _entities.reserveIdentity();
                ensurePendingSlot(entity.value());
                _pendingSpawns[entity.value()] = true;
                _commandQueue.emplace_back([entity, name](Registry& world) {
                    world.commitSpawn(entity, name);
                });
                return entity;
            }

            void commitSpawn(Entity entity, const std::string& name) {
                _entitiesDirty = true;
                _entities.commit(entity);
                _pendingSpawns[entity.value()] = false;
                _signatures.ensureEntity(entity.value());
                _signatures.clear(entity.value());
                if (!name.empty()) {
                    _entityNames[entity.value()] = name;
                }
            }

            void killEntityImmediate(Entity entity) {
                _entities.destroy(entity);
                for (const auto& pool : _pools) {
                    if (pool) {
                        pool->erase(entity.value());
                    }
                }
                _signatures.clear(entity.value());
                _entityNames.erase(entity.value());
                _entitiesDirty = true;
            }

            void killEntityDeferred(Entity entity) {
                _commandQueue.emplace_back([entity](Registry& world) {
                    if (!world._entities.isAlive(entity)) {
                        return;
                    }
                    world.killEntityImmediate(entity);
                });
            }

            template<typename StoredComponent>
            StoredComponent& addComponentImmediate(Entity entity, StoredComponent value) {
                auto& stored = ensureStorage<StoredComponent>().insertAt(entity.value(), std::move(value));
                _signatures.set(entity.value(), componentId<StoredComponent>());
                return stored;
            }

            template<typename StoredComponent>
            StoredComponent& addComponentDeferred(Entity entity, StoredComponent value) {
                ensureStorage<StoredComponent>();
                auto staged = std::make_shared<StoredComponent>(std::move(value));
                StoredComponent& ref = *staged;
                _commandQueue.emplace_back(
                    [entity, staged](Registry& world) {
                        if (!world._entities.isAlive(entity)) {
                            return;
                        }
                        world.addComponentImmediate<StoredComponent>(entity, std::move(*staged));
                    });
                return ref;
            }

            template<typename Component>
            Component& emplaceComponentImmediate(Entity entity, Component value) {
                auto& stored = ensureStorage<Component>().emplaceAt(entity.value(), std::move(value));
                _signatures.set(entity.value(), componentId<Component>());
                return stored;
            }

            template<typename Component>
            Component& emplaceComponentDeferred(Entity entity, Component value) {
                ensureStorage<Component>();
                auto staged = std::make_shared<Component>(std::move(value));
                Component& ref = *staged;
                _commandQueue.emplace_back(
                    [entity, staged](Registry& world) {
                        if (!world._entities.isAlive(entity)) {
                            return;
                        }
                        world.emplaceComponentImmediate<Component>(entity, std::move(*staged));
                    });
                return ref;
            }

            template<typename Component>
            void removeComponentImmediate(Entity entity) {
                const ComponentId id = componentId<Component>();
                if (id >= _pools.size() || !_pools[id]) {
                    return;
                }
                _pools[id]->erase(entity.value());
                // Safe: registering the pool widened the table for this id.
                _signatures.reset(entity.value(), id);
            }

            template<typename Component>
            void removeComponentDeferred(Entity entity) {
                _commandQueue.emplace_back([entity](Registry& world) {
                    if (!world._entities.isAlive(entity)) {
                        return;
                    }
                    world.removeComponentImmediate<Component>(entity);
                });
            }

            void flushCommands() {
                for (auto& command : _commandQueue) {
                    command(*this);
                }
                _commandQueue.clear();
            }

            void ensureEntityAlive(Entity entity) const {
                if (_entities.isAlive(entity)) {
                    return;
                }
                const std::size_t id = entity.value();
                const bool isPendingSpawn = id < _pendingSpawns.size()
                    && _pendingSpawns[id]
                    && entity == _entities.entityFromIndex(id);
                if (!isPendingSpawn) {
                    throw std::invalid_argument("ECS::Registry: entity handle is not alive");
                }
            }

            void ensurePendingSlot(std::size_t id) {
                if (id >= _pendingSpawns.size()) {
                    _pendingSpawns.resize(id + 1, false);
                }
            }

            void requireRegistered(ComponentId id) const {
                if (id >= _pools.size() || !_pools[id]) {
                    throw std::out_of_range("ECS::Registry: component not registered");
                }
            }

            /** @brief Registers Component if needed and returns its mutable storage. */
            template<typename Component>
            ComponentStorage<Component>& ensureStorage() {
                const ComponentId id = componentId<Component>();
                ensurePoolSlot(id);
                if (!_pools[id]) {
                    _signatures.ensureComponent(id);
                    _pools[id] = std::make_unique<ComponentPool<Component>>();
                }
                return typedPool<Component>(id).storage();
            }

            /** @brief Mutable storage of a registered component, for views and getComponent. */
            template<typename Component>
            ComponentStorage<Component>& mutableStorage() {
                const ComponentId id = componentId<Component>();
                requireRegistered(id);
                return typedPool<Component>(id).storage();
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

            void ensureResourceSlot(ResourceId id) {
                if (id >= _resources.size()) {
                    _resources.resize(static_cast<std::size_t>(id) + 1);
                }
            }

            void requireResourceSet(ResourceId id) const {
                if (id >= _resources.size() || !_resources[id]) {
                    throw std::out_of_range("ECS::Registry: resource not set");
                }
            }

            template<typename Resource>
            ResourceHolder<Resource>& typedResource(ResourceId id) {
                return *static_cast<ResourceHolder<Resource>*>(_resources[id].get());
            }

            template<typename Resource>
            const ResourceHolder<Resource>& typedResource(ResourceId id) const {
                return *static_cast<const ResourceHolder<Resource>*>(_resources[id].get());
            }

            EntityManager _entities;
            std::vector<std::unique_ptr<IComponentPool>> _pools;
            std::unordered_map<std::size_t, std::string> _entityNames;
            mutable std::vector<Entity> _cachedEntities;
            mutable bool _entitiesDirty{true};
            SignatureTable _signatures;
            std::vector<std::function<void(Registry&)>> _commandQueue;
            std::vector<bool> _pendingSpawns;
            std::vector<std::unique_ptr<IResourceHolder>> _resources;
            bool _deferring{false};
    };
}