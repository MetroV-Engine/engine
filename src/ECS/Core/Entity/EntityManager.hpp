#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <stdexcept>
#include <vector>

#include "Entity.hpp"

namespace ECS {
    namespace detail {
        // Starts at 1: InvalidWorld (0) is never issued.
        inline std::atomic<WorldId> nextWorldId{1};
    }

#ifdef ENGINE_TESTING
    namespace TestAccess {
        struct EntityManagerAccess;
    }
#endif

    /**
     * @brief Creates, validates and recycles entity identities.
     *
     * EntityManager owns the lifecycle state associated with Entity handles.
    * identities are recycled through a free list so creating many short-lived
    * entities does not grow the identity space unnecessarily.
     *
     * Each manager draws a unique WorldId and stamps it on every handle it
     * issues, so a handle from another manager is never alive here, even
     * when its index and generation happen to match a local entity.
     *
     * An identity is Free, Reserved (handed out by reserveIdentity() but not
     * yet visible) or Alive. Only Alive identities count in isAlive(),
     * size() and the list of entities.
     */
    class EntityManager {
#ifdef ENGINE_TESTING
        friend struct TestAccess::EntityManagerAccess;
#endif

        public:
            EntityManager() noexcept
                : _world(detail::nextWorldId.fetch_add(1, std::memory_order_relaxed)) {}

            // A copy would share this manager's WorldId, so each would accept
            // the other's handles.
            EntityManager(const EntityManager&) = delete;
            EntityManager& operator=(const EntityManager&) = delete;

            /**
             * @brief Creates a new live entity.
             * @return A handle for a recycled or newly allocated identity.
             * @throws std::overflow_error if no identity can be allocated.
             */
            Entity create() {
                const Entity entity = reserveIdentity();
                commit(entity);
                return entity;
            }

            /**
             * @brief Allocates an identity without marking it alive.
             * @return A handle for a recycled or newly allocated identity.
             * @throws std::overflow_error if no identity can be allocated.
             *
             * Pairs with commit(). Lets a caller hand out a stable, usable
             * handle before the identity should become visible to isAlive()
             * or snapshot() -- used by deferred entity creation, where the
             * handle must exist immediately but must not be observable
             * until the queued command actually applies.
             */
            Entity reserveIdentity() {
                std::size_t id = 0;
                if (!_freeIds.empty()) {
                    id = _freeIds.back();
                    _freeIds.pop_back();
                } else {
                    if (_nextId == std::numeric_limits<std::size_t>::max()) {
                        throw std::overflow_error("ECS::EntityManager: entity ID overflow");
                    }
                    id = _nextId++;
                    _states.push_back(State::Free);
                    _generations.push_back(0);
                }
                _states[id] = State::Reserved;
                ++_reservedCount;
                return Entity(id, _generations[id], _world);
            }

            /**
             * @brief Makes a reserved identity alive.
             * @throws std::invalid_argument if entity is not a reserved identity of this manager.
             */
            void commit(Entity entity) {
                if (!isReserved(entity)) {
                    throw std::invalid_argument("ECS::EntityManager: entity is not reserved");
                }
                _states[entity.index()] = State::Alive;
                --_reservedCount;
                _snapshotDirty = true;
            }

            /**
             * @brief Destroys a live entity and returns its identity to the free list.
             * @param entity Entity handle to destroy.
             * @throws std::invalid_argument if the handle is not currently live.
             *
             * The manager does not remove components. Registry owns that
             * responsibility because it knows which component pools exist.
             */
            void destroy(Entity entity) {
                const std::size_t id = entity.index();
                if (!isAlive(entity)) {
                    throw std::invalid_argument("ECS::EntityManager: entity is not alive");
                }

                if (_generations[id] == std::numeric_limits<EntityGeneration>::max()) {
                    throw std::overflow_error("ECS::EntityManager: entity generation overflow");
                }

                _states[id] = State::Free;
                ++_generations[id];
                _freeIds.push_back(id);
                _snapshotDirty = true;
            }

            /**
             * @brief Checks whether an entity currently belongs to this manager.
             * @param entity Handle to validate.
             * @return True when the handle was issued by this manager and
             *         its identity is allocated and live.
             */
            [[nodiscard]] bool isAlive(Entity entity) const noexcept {
                return hasState(entity, State::Alive);
            }

            /**
             * @brief Checks whether an entity was reserved by reserveIdentity()
             *        and not committed yet.
             * @return True when the handle was issued by this manager and
             *         its identity is reserved, with the same generation.
             */
            [[nodiscard]] bool isReserved(Entity entity) const noexcept {
                return hasState(entity, State::Reserved);
            }

            /**
             * @brief Returns the current handle for an allocated index.
             *
             * An index that was never allocated gets a handle carrying
             * InvalidWorld, which is never alive.
             */
            [[nodiscard]] Entity entityFromIndex(std::size_t id) const noexcept {
                if (id >= _generations.size()) {
                    return Entity(id, 0, InvalidWorld);
                }
                return Entity(id, _generations[id], _world);
            }

            /** @brief Returns the WorldId stamped on every handle this manager issues. */
            [[nodiscard]] WorldId worldId() const noexcept {
                return _world;
            }

            /**
             * @brief Returns the number of currently live entities.
             * @return Count of live entity identities; reserved ones are not counted.
             */
            [[nodiscard]] std::size_t size() const noexcept {
                return _nextId - _freeIds.size() - _reservedCount;
            }

            /**
             * @brief Returns the live entities as an immutable list, shared.
             *
             * Meant for tools, through EntityList (RegistryInspector::entities()).
             * Game systems iterate views instead.
             * @return Live entities in ascending identity order. The list is
             *         never modified: after a commit() or destroy(), the next
             *         call returns a new one, so a holder (an iteration in
             *         progress) keeps a consistent list. When nobody else holds
             *         the previous list, it is rebuilt in place instead.
             */
            [[nodiscard]] std::shared_ptr<const std::vector<Entity>> snapshot() const {
                if (!_snapshotDirty && _snapshot) {
                    return _snapshot;
                }
                if (!_snapshot || _snapshot.use_count() > 1) {
                    _snapshot = std::make_shared<std::vector<Entity>>();
                }
                _snapshot->clear();
                _snapshot->reserve(size());
                for (std::size_t id = 0; id < _nextId; ++id) {
                    if (_states[id] == State::Alive) {
                        _snapshot->push_back(Entity(id, _generations[id], _world));
                    }
                }
                _snapshotDirty = false;
                return _snapshot;
            }

            /**
             * @brief Reserves storage for entity lifecycle state.
             * @param capacity Number of identities to reserve.
             *
             * Reserving before a known bulk creation phase avoids repeated
             * allocations in the state vectors.
             */
            void reserve(std::size_t capacity) {
                _states.reserve(capacity);
                _generations.reserve(capacity);
                _freeIds.reserve(capacity);
            }

        private:
            enum class State : std::uint8_t { Free, Reserved, Alive };

            [[nodiscard]] bool hasState(Entity entity, State state) const noexcept {
                const std::size_t id = entity.index();
                return entity.world() == _world
                    && id < _states.size()
                    && _states[id] == state
                    && _generations[id] == entity.generation();
            }

            std::vector<State> _states;
            std::vector<EntityGeneration> _generations;
            std::vector<std::size_t> _freeIds;
            std::size_t _nextId{0};
            std::size_t _reservedCount{0};
            WorldId _world;
            // Cache of the live entities list, see snapshot().
            mutable std::shared_ptr<std::vector<Entity>> _snapshot;
            mutable bool _snapshotDirty{true};
    };
}