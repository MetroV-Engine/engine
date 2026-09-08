#pragma once

#include <cstddef>
#include <limits>
#include <stdexcept>
#include <vector>

#include "Entity.hpp"

namespace ECS {
    /**
     * @brief Creates, validates and recycles entity identities.
     *
     * EntityManager owns the lifecycle state associated with Entity handles.
    * identities are recycled through a free list so creating many short-lived
    * entities does not grow the identity space unnecessarily.
     */
    class EntityManager {
        public:
            /**
             * @brief Creates a new live entity.
             * @return A handle for a recycled or newly allocated identity.
             * @throws std::overflow_error if no identity can be allocated.
             */
            Entity create() {
                if (!_freeIds.empty()) {
                    const std::size_t id = _freeIds.back();
                    _freeIds.pop_back();
                    _alive[id] = true;
                    return Entity(id, _generations[id]);
                }

                if (_nextId == std::numeric_limits<std::size_t>::max()) {
                    throw std::overflow_error("ECS::EntityManager: entity ID overflow");
                }

                const std::size_t id = _nextId++;
                _alive.push_back(true);
                _generations.push_back(0);
                return Entity(id, _generations.back());
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
                const std::size_t id = entity.value();
                if (!isAlive(entity)) {
                    throw std::invalid_argument("ECS::EntityManager: entity is not alive");
                }

                if (_generations[id] == std::numeric_limits<EntityGeneration>::max()) {
                    throw std::overflow_error("ECS::EntityManager: entity generation overflow");
                }

                _alive[id] = false;
                ++_generations[id];
                _freeIds.push_back(id);
            }

            /**
             * @brief Checks whether an entity currently belongs to this manager.
             * @param entity Handle to validate.
             * @return True when the identity is allocated and live.
             */
            [[nodiscard]] bool isAlive(Entity entity) const noexcept {
                const std::size_t id = entity.value();
                return id < _alive.size()
                    && _alive[id]
                    && _generations[id] == entity.generation();
            }

            /** @brief Returns the current handle for an allocated index. */
            [[nodiscard]] Entity entityFromIndex(std::size_t id) const noexcept {
                if (id >= _generations.size()) {
                    return Entity(id);
                }
                return Entity(id, _generations[id]);
            }

            /**
             * @brief Returns the number of currently live entities.
             * @return Count of live entity identities.
             */
            [[nodiscard]] std::size_t size() const noexcept {
                return _nextId - _freeIds.size();
            }

            /**
             * @brief Returns handles for every currently live identity.
             * @return Live entities in ascending identity order.
             *
             * This helper is intended for editor-style enumeration. Hot
             * systems should iterate component pools instead of rebuilding this
             * list every frame.
             */
            [[nodiscard]] std::vector<Entity> getAll() const {
                std::vector<Entity> entities;
                entities.reserve(size());
                for (std::size_t id = 0; id < _nextId; ++id) {
                    if (_alive[id]) {
                        entities.emplace_back(id, _generations[id]);
                    }
                }
                return entities;
            }

            /**
             * @brief Reserves storage for entity lifecycle state.
             * @param capacity Number of identities to reserve.
             *
             * Reserving before a known bulk creation phase avoids repeated
             * allocations in the alive-state vector.
             */
            void reserve(std::size_t capacity) {
                _alive.reserve(capacity);
                _generations.reserve(capacity);
                _freeIds.reserve(capacity);
            }

        private:
            std::vector<bool> _alive;
            std::vector<EntityGeneration> _generations;
            std::vector<std::size_t> _freeIds;
            std::size_t _nextId{0};
    };
}