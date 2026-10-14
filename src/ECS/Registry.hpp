#pragma once

#include <cstddef>
#include <functional>
#include <initializer_list>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "Core/Component/ComponentStore.hpp"
#include "Core/Entity/EntityManager.hpp"
#include "Core/Entity/Name.hpp"
#include "Core/CommandBuffer.hpp"
#include "Core/Resource/ResourceStore.hpp"
#include "View.hpp"

namespace ECS {
    namespace reflect {
        class RegistryInspector;
    }

#ifdef ENGINE_TESTING
    namespace TestAccess {
        struct RegistryIdAccess;
    }
#endif

    /**
     * @brief The world: entities, their components and the resources.
     *
     * A facade over its parts: EntityManager (entity lifecycle),
     * ComponentStore (pools and signature rows), ResourceStore and
     * CommandBuffer (deferred mode). It checks that entities are alive,
     * chooses between applying a change now or at flush, and keeps the parts
     * consistent (killing an entity erases its components).
     *
     * Storages are handed out read-only: adding or removing a component must
     * go through the Registry, so the pools and the signature rows can't
     * disagree.
     */
    class Registry {
        public:
            Registry() = default;
            Registry(const Registry&) = delete;
            Registry& operator=(const Registry&) = delete;

            /**
             * @brief Creates an entity, with a Name component when name is not empty.
             * @param name Optional display name, stored as an ECS::Name component.
             * @return Newly created entity handle. Under runDeferred, the
             *         entity (and its Name) only exists after the flush, but
             *         the handle can already receive components.
             */
            Entity spawnEntity(const std::string& name = {}) {
                if (_commands.recording()) {
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
             * @brief Checks whether an entity handle refers to a live entity of this registry.
             *
             * For handles kept across frames (a target, a parent): the entity
             * may have been killed since. An entity spawned under runDeferred
             * is not alive until the flush.
             */
            [[nodiscard]] bool isAlive(Entity entity) const noexcept {
                return _entities.isAlive(entity);
            }

            /** @brief Number of live entities; spawns still queued under runDeferred are not counted. */
            [[nodiscard]] std::size_t entityCount() const noexcept {
                return _entities.size();
            }

            /**
             * @brief Destroys an entity and removes it from every component pool.
             * @param entity Entity to destroy.
             * @throws std::invalid_argument if entity is not alive. Under
             *         runDeferred the kill is queued instead, and ignored at
             *         flush if the entity is already dead.
             */
            void killEntity(Entity entity) {
                mutate(entity, [this, entity] { killEntityImmediate(entity); });
            }

            /**
             * @brief Registers a component pool if absent and returns its storage.
             * @tparam Component Component type to register.
             * @return Read-only typed dense storage for Component.
             */
            template<typename Component>
            const ComponentStorage<Component>& registerComponent() {
                return _components.ensure<Component>();
            }

            /**
             * @brief Returns an already registered component storage, read-only.
             * @throws std::out_of_range when Component has not been registered.
             */
            template<typename Component>
            const ComponentStorage<Component>& getComponents() const {
                return _components.storage<Component>();
            }

            /**
             * @brief Returns a component for an entity.             *
             * The reference is short-lived: it is valid until the next add or
             * remove of this component type (the storage is dense and may
             * move its elements), or, under runDeferred, until the end of the
             * outermost runDeferred. To keep something across frames, keep
             * the Entity and fetch the component again.
             * @throws std::out_of_range if the pool or component is absent.
             */
            template<typename Component>
            Component& getComponent(Entity entity) {
                ensureEntityAlive(entity);
                return _components.storage<Component>().get(entity.value());
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
             * @return Reference to the stored component; under runDeferred, to
             *         the value that will be applied at flush. See getComponent
             *         for how long it stays valid.
             */
            template<typename Component>
            std::decay_t<Component>& addComponent(Entity entity, Component&& component) {
                ensureEntityAlive(entity);
                using StoredComponent = std::decay_t<Component>;
                StoredComponent value(std::forward<Component>(component));
                return addOrStage(entity, std::move(value), [this, entity](StoredComponent&& added) -> StoredComponent& {
                    return _components.insert<StoredComponent>(entity.value(), std::move(added));
                });
            }

            /**
             * @brief Constructs a component from params, then adds or replaces it like addComponent.
             * @return Same as addComponent.
             */
            template<typename Component, typename... Params>
            Component& emplaceComponent(Entity entity, Params&&... params) {
                return addComponent(entity, Component(std::forward<Params>(params)...));
            }

            /**
             * @brief Removes a component from an entity when its pool exists.
             * @param entity Entity whose component should be removed.
             */
            template<typename Component>
            void removeComponent(Entity entity) {
                removeComponentById(entity, componentId<Component>());
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
                return hasComponentsById(entity, {componentId<Components>()...});
            }

            /**
             * @brief Returns a read-only typed storage when registered, otherwise nullptr.
             */
            template<typename Component>
            const ComponentStorage<Component>* getIf() const noexcept {
                return _components.find<Component>();
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
                return View<Components...>(_entities, _components.ensure<Components>()...);
            }

            /**
             * @brief Creates a read-only query over entities with all requested components.
             *
             * Like the mutable overload, a Component never registered simply
             * matches no entity. Being const, this does not register it: it
             * reads an empty storage instead.
             */
            template<typename... Components>
            ReadOnlyView<Components...> view() const {
                return ReadOnlyView<Components...>(_entities, _components.storageOrEmpty<Components>()...);
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
             * deferred mode. A nested runDeferred joins the outer one: its
             * commands apply when the outermost call ends.
             */
            template<typename Callable>
            void runDeferred(Callable&& callable) {
                _commands.record(std::forward<Callable>(callable));
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
                return _resources.set<Resource>(std::forward<Params>(params)...);
            }

            /**
             * @brief Returns a previously set resource.
             * @throws std::out_of_range when Resource has never been set.
             */
            template<typename Resource>
            [[nodiscard]] Resource& getResource() {
                return _resources.get<Resource>();
            }

            /** @copydoc getResource() */
            template<typename Resource>
            [[nodiscard]] const Resource& getResource() const {
                return _resources.get<Resource>();
            }

            /** @brief Checks whether a resource of this type has been set. */
            template<typename Resource>
            [[nodiscard]] bool hasResource() const noexcept {
                return _resources.has<Resource>();
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
                _components.onRemove<Component>(std::move(callback));
            }

            /**
             * @brief Access to this registry's components without knowing their types.
             *
             * For code that only has a component's TypeInfo (editor, scripting,
             * scene loading, serialization). Code that knows its types uses the
             * typed API above instead.
             *
             * Returns a lightweight handle referencing this registry: create it
             * where needed, it must not outlive the registry. Defined in
             * ECS/Reflection/RegistryInspector.hpp, which must be included to
             * call it (the ECS core does not depend on the reflection layer).
             */
            [[nodiscard]] reflect::RegistryInspector inspect();

        private:
            friend class reflect::RegistryInspector;
#ifdef ENGINE_TESTING
            friend struct TestAccess::RegistryIdAccess;
#endif

            // ----------------------------------------------------------------
            // ComponentId-based access (private)
            //
            // The same operations as the typed API, for code that only knows
            // a component's ComponentId. They are the single implementation
            // of has/remove, which the typed API forwards to. Only
            // reflect::RegistryInspector (a friend) uses them. They check the
            // entity, then forward to the ComponentStore.
            // ----------------------------------------------------------------

            /**
             * @brief Creates the pool for id with factory, unless this registry already has one.
             * @param factory Must create the pool of the type whose componentId() is id.
             */
            void ensurePoolById(ComponentId id, PoolFactory factory) {
                _components.ensurePool(id, factory);
            }

            /** @brief Checks whether this registry has a pool for id. */
            [[nodiscard]] bool hasPoolById(ComponentId id) const noexcept {
                return _components.hasPool(id);
            }

            /**
             * @brief Checks whether an entity owns every listed component.
             * @return True when the entity is alive and its signature row has
             *         every id's bit set. A type never registered here has no
             *         bit set for any entity.
             */
            [[nodiscard]] bool hasComponentsById(Entity entity, std::initializer_list<ComponentId> ids) const {
                return _entities.isAlive(entity) && _components.has(entity.value(), ids);
            }

            /**
             * @brief Address of an entity's component, or nullptr when absent.
             * @param ops Operations of the type whose componentId() is id.
             * @throws std::invalid_argument if entity is not alive.
             */
            [[nodiscard]] void* getComponentById(Entity entity, ComponentId id, const ComponentOps& ops) {
                ensureEntityAlive(entity);
                return _components.get(entity.value(), id, ops);
            }

            /**
             * @brief Adds (or replaces) a default-constructed component.
             * @return Address of the component; under runDeferred, of the
             *         value that will be moved in at flush (like addComponent).
             *         nullptr when the type has no default constructor.
             * @param ops Operations of the type whose componentId() is id.
             * @throws std::invalid_argument if entity is not alive.
             * @throws std::out_of_range when this registry has no pool for id.
             */
            void* addDefaultComponentById(Entity entity, ComponentId id, const ComponentOps& ops) {
                ensureEntityAlive(entity);
                if (!_components.hasPool(id)) {
                    throw std::out_of_range("ECS::Registry: component not registered");
                }
                void* raw = ops.newDefault();
                if (!raw) {
                    return nullptr;
                }
                // Owns the value until it is moved into the pool (now or at flush).
                std::shared_ptr<void> value(raw, ops.deleteValue);
                if (!_commands.recording()) {
                    return _components.insertMoved(entity.value(), id, value.get(), ops);
                }
                mutate(entity, [this, entity, id, value, ops] {
                    _components.insertMoved(entity.value(), id, value.get(), ops);
                });
                return value.get();
            }

            /**
             * @brief Removes an entity's component when this registry has its pool.
             * @throws std::invalid_argument if entity is not alive.
             */
            void removeComponentById(Entity entity, ComponentId id) {
                ensureEntityAlive(entity);
                mutate(entity, [this, entity, id] { _components.remove(entity.value(), id); });
            }

            /**
             * @brief ComponentIds of every component an entity owns, ascending.
             * @return Empty when the entity is not alive.
             */
            [[nodiscard]] std::vector<ComponentId> componentIdsOf(Entity entity) const {
                if (!_entities.isAlive(entity)) {
                    return {};
                }
                return _components.idsOf(entity.value());
            }

            Entity spawnEntityImmediate(const std::string& name) {
                const Entity entity = _entities.create();
                _components.addEntity(entity.value());
                addName(entity, name);
                return entity;
            }

            Entity spawnEntityDeferred(const std::string& name) {
                const Entity entity = _entities.reserveIdentity();
                // Not through mutate: the entity only becomes alive in this command.
                _commands.push([this, entity, name] { commitSpawn(entity, name); });
                return entity;
            }

            void addName(Entity entity, const std::string& name) {
                if (!name.empty()) {
                    _components.insert(entity.value(), Name{name});
                }
            }

            void commitSpawn(Entity entity, const std::string& name) {
                _entities.commit(entity);
                _components.addEntity(entity.value());
                addName(entity, name);
            }

            void killEntityImmediate(Entity entity) {
                _entities.destroy(entity);
                _components.removeEntity(entity.value());
            }

            /**
             * @brief Applies fn now, or queues it while runDeferred records.
             *
             * A queued fn only runs if entity is still alive at flush, so a
             * batch that kills an entity ignores the later changes to it.
             * Commands capture this: a Registry is neither copied nor moved.
             */
            template<typename Fn>
            void mutate(Entity entity, Fn&& fn) {
                if (!_commands.recording()) {
                    fn();
                    return;
                }
                _commands.push([this, entity, fn = std::forward<Fn>(fn)]() mutable {
                    if (_entities.isAlive(entity)) {
                        fn();
                    }
                });
            }

            /**
             * @brief Adds value now, or keeps it until flush while runDeferred records.
             * @param insert Moves its argument into the store and returns the stored component.
             * @return The stored component; under runDeferred, the staged value,
             *         so the caller can still modify it before it is applied.
             *
             * When deferred, the pool is created at once, so the type is
             * registered (and queryable) as soon as the add is requested.
             */
            template<typename Component, typename Insert>
            Component& addOrStage(Entity entity, Component value, Insert insert) {
                if (!_commands.recording()) {
                    return insert(std::move(value));
                }
                _components.ensure<Component>();
                auto staged = std::make_shared<Component>(std::move(value));
                mutate(entity, [staged, insert]() mutable { insert(std::move(*staged)); });
                return *staged;
            }

            // Accepts an entity spawned under runDeferred and not flushed
            // yet, so it can already receive components (applied in order).
            void ensureEntityAlive(Entity entity) const {
                if (!_entities.isAlive(entity) && !_entities.isReserved(entity)) {
                    throw std::invalid_argument("ECS::Registry: entity handle is not alive");
                }
            }

            EntityManager _entities;
            ComponentStore _components;
            ResourceStore _resources;
            CommandBuffer _commands;
    };
}