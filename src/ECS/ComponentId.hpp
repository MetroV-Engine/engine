#pragma once

#include <atomic>
#include <cstdint>

namespace ECS {
    using ComponentId = std::uint32_t;

    namespace detail {
        inline std::atomic<ComponentId> nextComponentId{0};
    }

    /**
     * @brief Returns the process-local numeric ID assigned to a component type.
     * @tparam Component Component type that needs an ECS pool.
     * @return Stable ID for Component during this process.
     *
     * The ID is allocated lazily: the first call for a type consumes one slot,
     * and later calls return the same function-local static value. No manual
     * registration line is required for each component. IDs are runtime details
     * and must not be persisted in scene files or network data.
     */
    template<typename Component>
    [[nodiscard]] ComponentId componentId() noexcept {
        static const ComponentId id =
            detail::nextComponentId.fetch_add(1, std::memory_order_relaxed);
        return id;
    }
}