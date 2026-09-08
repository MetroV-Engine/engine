#pragma once

#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>

#include "ComponentStorage.hpp"
#include "Entity.hpp"
#include "Zipper.hpp"

namespace ECS {
    namespace detail {
        /**
         * @brief Adapts a zipper iterator to the public view tuple.
         *
         * Zipper keeps the entity ID at the end of its internal tuple. View
         * moves it to the beginning and wraps it in Entity for its public API.
         */
        template<typename ZipperIterator, typename ValueType, std::size_t ComponentCount>
        class ViewIterator {
            public:
                ViewIterator() = delete;

                explicit ViewIterator(ZipperIterator zipperIterator)
                    : _iterator(std::move(zipperIterator)) {}

                ViewIterator& operator++() {
                    ++_iterator;
                    return *this;
                }

                [[nodiscard]] bool operator!=(const ViewIterator& other) const {
                    return _iterator != other._iterator;
                }

                /** @brief Returns the entity and references to its components. */
                ValueType operator*() const {
                    const auto values = *_iterator;
                    return makeValue(values, std::make_index_sequence<ComponentCount>{});
                }

            private:
                template<std::size_t... Indices>
                static ValueType makeValue(
                    const auto& values,
                    std::index_sequence<Indices...>) {
                    return ValueType(
                        Entity(std::get<ComponentCount>(values)),
                        std::get<Indices>(values)...);
                }

                ZipperIterator _iterator;
        };

        /**
         * @brief Shared implementation for mutable and read-only component views.
         * @tparam ReadOnly Whether the view exposes const component references.
         * @tparam Components Component types required by the query.
         */
        template<bool ReadOnly, typename... Components>
        class ViewImpl {
            private:
                template<typename Component>
                using Storage = std::conditional_t<
                    ReadOnly,
                    const ComponentStorage<std::remove_const_t<Component>>,
                    ComponentStorage<std::remove_const_t<Component>>>;

                template<typename Component>
                using StorageReference = Storage<Component>&;

                using StorageTuple = std::tuple<StorageReference<Components>...>;

                using ZipperType = decltype(std::apply(
                    [](auto&... storages) { return zipper(storages...); },
                    std::declval<StorageTuple&>()));

                using ZipperIterator = decltype(
                    std::declval<ZipperType&>().begin());

            public:
                using value_type = std::tuple<
                    Entity,
                    decltype(std::declval<StorageReference<Components>>().get(
                        std::size_t{}))...>;
                using iterator = ViewIterator<
                    ZipperIterator,
                    value_type,
                    sizeof...(Components)>;

                /**
                 * @brief Creates a view over the supplied component storages.
                 * @param storages Storages queried for a common entity set.
                 */
                explicit ViewImpl(StorageReference<Components>... storages)
                    : _storages(storages...) {}

                /** @brief Returns an iterator to the first matching entity. */
                iterator begin() {
                    return makeIterator([](auto&... storages) {
                        return zipper(storages...).begin();
                    });
                }

                /** @brief Returns the end iterator for this view. */
                iterator end() {
                    return makeIterator([](auto&... storages) {
                        return zipper(storages...).end();
                    });
                }

            private:
                template<typename Factory>
                iterator makeIterator(Factory&& factory) {
                    return iterator(std::apply(
                        std::forward<Factory>(factory),
                        _storages));
                }

                StorageTuple _storages;
        };
    }

    /** @brief Mutable query over entities owning every requested component. */
    template<typename... Components>
    using View = detail::ViewImpl<false, Components...>;

    /** @brief Read-only query over entities owning every requested component. */
    template<typename... Components>
    using ReadOnlyView = detail::ViewImpl<true, Components...>;
}
