#pragma once

#include <cstddef>
#include <tuple>
#include <utility>

namespace ECS {
    /**
    * @brief Iterates the entities shared by several component storages.
    * @tparam FirstSet Storage used as the driving dense iteration.
    * @tparam OtherSets Additional storages checked for entity membership.
     *
     * The first set determines iteration cost. For every packed component in
     * it, the zipper checks membership in the other sets and yields only the
     * intersection. The returned tuple contains component references followed
     * by the matching entity identity.
     */
    template<typename FirstSet, typename... OtherSets>
    class ZipperN {
        public:
            using EntityType = typename FirstSet::size_type;
            using value_type = std::tuple<
                decltype(std::declval<FirstSet&>().get(std::size_t{})),
                decltype(std::declval<OtherSets&>().get(std::size_t{}))...,
                EntityType>;

            /**
             * @brief Creates an intersection query over component storages.
             * @param first Storage used to drive iteration.
             * @param others Storages whose entity membership is required.
             */
            ZipperN(FirstSet& first, OtherSets&... others)
                : _first(first), _others(others...) {}

            /** @brief Iterator over matching entity/component tuples. */
            class iterator {
                public:
                    iterator(FirstSet& first,
                             std::tuple<OtherSets&...> others,
                             std::size_t index)
                        : _first(first), _others(others), _index(index) {
                        advance();
                    }

                    /** @brief Advances to the next entity present in every set. */
                    iterator& operator++() {
                        ++_index;
                        advance();
                        return *this;
                    }

                    /** @brief Compares packed positions of two iterators. */
                    [[nodiscard]] bool operator!=(const iterator& other) const {
                        return _index != other._index;
                    }

                    /**
                     * @brief Returns references to matching components and entity ID.
                     * @return Tuple in the order FirstSet, OtherSets..., entity.
                     */
                    value_type operator*() const {
                        const EntityType entity = _first.entityAt(_index);
                        return dereference(entity,
                                           std::index_sequence_for<OtherSets...>{});
                    }

                private:
                    void advance() {
                        while (_index < _first.size()) {
                            const EntityType entity = _first.entityAt(_index);
                            if (has_all(entity,
                                        std::index_sequence_for<OtherSets...>{})) {
                                return;
                            }
                            ++_index;
                        }
                    }

                    template<std::size_t... Indices>
                    [[nodiscard]] bool has_all(
                        EntityType entity,
                        std::index_sequence<Indices...>) const {
                        return (... && std::get<Indices>(_others).has(entity));
                    }

                    template<std::size_t... Indices>
                    value_type dereference(
                        EntityType entity,
                        std::index_sequence<Indices...>) const {
                        return value_type(
                            _first[_index],
                            std::get<Indices>(_others).get(entity)...,
                            entity);
                    }

                    FirstSet& _first;
                    std::tuple<OtherSets&...> _others;
                    std::size_t _index;
            };

            /** @brief Returns an iterator to the first matching entity. */
            iterator begin() { return iterator(_first, _others, 0); }

            /** @brief Returns the sentinel iterator after the driving set. */
            iterator end() { return iterator(_first, _others, _first.size()); }

        private:
            FirstSet& _first;
            std::tuple<OtherSets&...> _others;
    };

    /**
     * @brief Deduction helper for constructing a zipper without template arguments.
     * @return A zipper over first and all other component sets.
     */
    template<typename FirstSet, typename... OtherSets>
    ZipperN<FirstSet, OtherSets...> zipper(FirstSet& first, OtherSets&... others) {
        return ZipperN<FirstSet, OtherSets...>(first, others...);
    }
}