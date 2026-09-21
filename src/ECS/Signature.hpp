#pragma once

#include <bitset>
#include <cstddef>

namespace ECS {
    /**
     * @brief Maximum number of distinct component types trackable in a Signature.
     *
     * This is a process-wide limit: componentId<T>() assigns one id per
     * distinct C++ type used anywhere in the program, not per entity or per
     * component instance. Sized well above a full game's expected component
     * type count (Hollow Knight-scale games are estimated around 40-70) to
     * leave headroom for scripted/user-defined components. Registry refuses a
     * component type past this limit with std::out_of_range when it is
     * registered, before any entity data is touched, rather than silently
     * corrupting state.
     */
    constexpr std::size_t MaxComponentTypes = 256;

    /** @brief Bitset marking which component types an entity currently owns. */
    using Signature = std::bitset<MaxComponentTypes>;
}
