#pragma once

#include <cstddef>

#include "ECS/Entity.hpp"

namespace ECS::TestAccess {
    /** @brief Builds arbitrary handles, which only EntityManager may do in production. */
    struct EntityAccess {
        static Entity make(
            std::size_t index,
            EntityGeneration generation,
            WorldId world = InvalidWorld
        ) {
            return Entity(index, generation, world);
        }
    };
}
