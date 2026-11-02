#pragma once

#include <atomic>
#include <cstdint>

namespace ECS {
    using ResourceId = std::uint32_t;

    namespace detail {
        inline std::atomic<ResourceId> nextResourceId{0};
    }

    /**
     * @brief Returns the process-local numeric ID assigned to a resource type.
     * @tparam Resource Resource type stored on a Registry.
     * @return Stable ID for Resource during this process.
     *
     * The ID is allocated lazily: the first call for a type consumes one slot,
     * and later calls return the same function-local static value. This is a
     * separate ID space from componentId<T>(): a resource and a component can
     * share the same T without colliding.
     */
    template<typename Resource>
    [[nodiscard]] ResourceId resourceId() noexcept {
        static const ResourceId id =
            detail::nextResourceId.fetch_add(1, std::memory_order_relaxed);
        return id;
    }
}
