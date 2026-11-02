#pragma once

#include <cstddef>
#include <utility>
#include <memory>
#include <vector>

#include "Entity.hpp"
#include "EntityManager.hpp"

namespace ECS {
    /**
     * @brief Always up-to-date list of the live entities, for tools.
     *
     * Holds no entity itself, only the EntityManager: keep it as long as
     * needed (an editor panel keeps one for its whole life) and iterate it
     * whenever the current entities are needed, without fetching a new list
     * after spawns or kills.
     *
     * Each iteration takes the current EntityManager::snapshot() and keeps it
     * until the iteration ends, so changing the world during a loop never
     * disturbs that loop: it finishes on the entities it started with, and
     * the next loop sees the new ones. Entities killed meanwhile are skipped.
     *
     * Must not outlive the EntityManager (in practice, the Registry).
     */
    class EntityList {
        public:
            /** @brief End of an iteration; compared with Iterator. */
            struct Sentinel {};

            /** @brief Walks one snapshot, skipping the entities that are no longer alive. */
            class Iterator {
                public:
                    using value_type = Entity;
                    using difference_type = std::ptrdiff_t;

                    Iterator() = default;

                    Iterator(const EntityManager& manager,
                             std::shared_ptr<const std::vector<Entity>> snapshot)
                        : _manager(&manager), _snapshot(std::move(snapshot)) {
                        skipDead();
                    }

                    [[nodiscard]] Entity operator*() const { return (*_snapshot)[_index]; }

                    Iterator& operator++() {
                        ++_index;
                        skipDead();
                        return *this;
                    }

                    Iterator operator++(int) {
                        Iterator previous = *this;
                        ++*this;
                        return previous;
                    }

                    [[nodiscard]] friend bool operator==(const Iterator& it, Sentinel) noexcept {
                        return !it._snapshot || it._index >= it._snapshot->size();
                    }

                private:
                    void skipDead() {
                        while (_index < _snapshot->size() && !_manager->isAlive((*_snapshot)[_index])) {
                            ++_index;
                        }
                    }

                    const EntityManager* _manager = nullptr;
                    std::shared_ptr<const std::vector<Entity>> _snapshot;
                    std::size_t _index = 0;
            };

            explicit EntityList(const EntityManager& manager) noexcept : _manager(&manager) {}

            /** @brief Starts an iteration over the current live entities. */
            [[nodiscard]] Iterator begin() const { return Iterator(*_manager, _manager->snapshot()); }

            [[nodiscard]] Sentinel end() const noexcept { return {}; }

            /** @brief Number of live entities right now. */
            [[nodiscard]] std::size_t size() const noexcept { return _manager->size(); }

            /** @brief True when no entity is alive right now. */
            [[nodiscard]] bool empty() const noexcept { return size() == 0; }

        private:
            const EntityManager* _manager;
    };
}
