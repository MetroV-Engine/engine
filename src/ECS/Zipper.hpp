#pragma once

#include <cstddef>
#include <tuple>
#include <utility>
#include <vector>

namespace ECS {
    /**
     * @brief Iterates the entities shared by several component storages.
     * @tparam Sets Component storages whose entity intersection is queried.
     *
     * Iteration is driven by whichever storage is smallest when `begin()` or
     * `end()` is called: its dense entity list is walked, and every other
     * storage is checked for membership. Ties go to the storage listed first.
     * The cost of a full iteration is therefore proportional to the smallest
     * storage, regardless of the order the storages are passed in.
     *
     * The yielded tuple always follows the argument order — component
     * references for each storage, then the matching entity identity — no
     * matter which storage drives. Iteration order follows the driving
     * storage's dense order, which is not stable across removals.
     */
    template<typename... Sets>
    class ZipperN {
        static_assert(sizeof...(Sets) > 0, "ECS::ZipperN requires at least one storage");

        public:
            using EntityType = std::size_t;
            using value_type = std::tuple<
                decltype(std::declval<Sets&>().get(std::size_t{}))...,
                EntityType>;

            /**
             * @brief Creates an intersection query over component storages.
             * @param sets Storages whose entity membership is required.
             */
            explicit ZipperN(Sets&... sets) : _sets(sets...) {}

            /** @brief Iterator over matching entity/component tuples. */
            class iterator {
                public:
                    iterator(std::tuple<Sets&...> sets,
                             std::size_t driver,
                             const std::vector<EntityType>* driverEntities,
                             std::size_t index)
                        : _sets(sets),
                          _driver(driver),
                          _driverEntities(driverEntities),
                          _index(index) {
                        advance();
                    }

                    /** @brief Advances to the next entity present in every set. */
                    iterator& operator++() {
                        ++_index;
                        advance();
                        return *this;
                    }

                    /** @brief Compares dense positions in the driving storage. */
                    [[nodiscard]] bool operator!=(const iterator& other) const {
                        return _index != other._index;
                    }

                    /**
                     * @brief Returns references to matching components and entity ID.
                     * @return Tuple in argument order, followed by the entity.
                     */
                    value_type operator*() const {
                        const EntityType entity = _driverEntities->at(_index);
                        return dereference(entity, std::index_sequence_for<Sets...>{});
                    }

                private:
                    void advance() {
                        while (_index < _driverEntities->size()) {
                            if (hasAll((*_driverEntities)[_index],
                                       std::index_sequence_for<Sets...>{})) {
                                return;
                            }
                            ++_index;
                        }
                    }

                    /** @brief Checks every storage except the driver, which owns the entity by construction. */
                    template<std::size_t... Indices>
                    [[nodiscard]] bool hasAll(EntityType entity,
                                              std::index_sequence<Indices...>) const {
                        return (... && (Indices == _driver
                                        || std::get<Indices>(_sets).has(entity)));
                    }

                    template<std::size_t... Indices>
                    value_type dereference(EntityType entity,
                                           std::index_sequence<Indices...>) const {
                        return value_type(component<Indices>(entity)..., entity);
                    }

                    /** @brief The driver is read by dense position; the others by entity lookup. */
                    template<std::size_t Index>
                    decltype(auto) component(EntityType entity) const {
                        auto& set = std::get<Index>(_sets);
                        return Index == _driver ? set[_index] : set.get(entity);
                    }

                    std::tuple<Sets&...> _sets;
                    std::size_t _driver;
                    const std::vector<EntityType>* _driverEntities;
                    std::size_t _index;
            };

            /** @brief Returns an iterator to the first matching entity. */
            iterator begin() {
                const Driver driver = selectDriver(std::index_sequence_for<Sets...>{});
                return iterator(_sets, driver.index, driver.entities, 0);
            }

            /** @brief Returns the sentinel iterator after the driving storage. */
            iterator end() {
                const Driver driver = selectDriver(std::index_sequence_for<Sets...>{});
                return iterator(_sets, driver.index, driver.entities, driver.entities->size());
            }

        private:
            struct Driver {
                std::size_t index;
                const std::vector<EntityType>* entities;
            };

            /** @brief Picks the smallest storage; strict comparison keeps ties on the earliest one. */
            template<std::size_t... Indices>
            [[nodiscard]] Driver selectDriver(std::index_sequence<Indices...>) const {
                Driver driver{0, &std::get<0>(_sets).entities()};
                std::size_t smallest = std::get<0>(_sets).size();
                const auto consider = [&](std::size_t index, const auto& set) {
                    if (set.size() < smallest) {
                        smallest = set.size();
                        driver = Driver{index, &set.entities()};
                    }
                };
                (consider(Indices, std::get<Indices>(_sets)), ...);
                return driver;
            }

            std::tuple<Sets&...> _sets;
    };

    /**
     * @brief Deduction helper for constructing a zipper without template arguments.
     * @return A zipper over all given component storages.
     */
    template<typename... Sets>
    ZipperN<Sets...> zipper(Sets&... sets) {
        return ZipperN<Sets...>(sets...);
    }
}
