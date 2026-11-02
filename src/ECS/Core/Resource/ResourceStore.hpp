#pragma once

#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

#include "ResourceHolder.hpp"
#include "ResourceId.hpp"

namespace ECS {
    /**
     * @brief Holds at most one value per resource type.
     *
     * Resources are world-wide singletons (time, input, settings...) that do
     * not belong to an entity. Each type is indexed by its resourceId(), a
     * separate ID space from components, so a type can be both a component
     * and a resource without colliding.
     */
    class ResourceStore {
        public:
            /**
             * @brief Constructs or replaces the Resource-typed value.
             * @tparam Resource Resource type.
             * @param params Constructor arguments forwarded to Resource.
             * @return Reference to the stored resource. Replacing a resource
             *         invalidates references to the previous value.
             */
            template<typename Resource, typename... Params>
            Resource& set(Params&&... params) {
                const ResourceId id = resourceId<Resource>();
                ensureSlot(id);
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
            [[nodiscard]] Resource& get() {
                const ResourceId id = resourceId<Resource>();
                requireSet(id);
                return static_cast<ResourceHolder<Resource>&>(*_resources[id]).value();
            }

            /** @copydoc get() */
            template<typename Resource>
            [[nodiscard]] const Resource& get() const {
                const ResourceId id = resourceId<Resource>();
                requireSet(id);
                return static_cast<const ResourceHolder<Resource>&>(*_resources[id]).value();
            }

            /** @brief Checks whether a resource of this type has been set. */
            template<typename Resource>
            [[nodiscard]] bool has() const noexcept {
                const ResourceId id = resourceId<Resource>();
                return id < _resources.size() && _resources[id] != nullptr;
            }

        private:
            void ensureSlot(ResourceId id) {
                if (id >= _resources.size()) {
                    _resources.resize(static_cast<std::size_t>(id) + 1);
                }
            }

            void requireSet(ResourceId id) const {
                if (id >= _resources.size() || !_resources[id]) {
                    throw std::out_of_range("ECS::ResourceStore: resource not set");
                }
            }

            std::vector<std::unique_ptr<IResourceHolder>> _resources;
    };
}
