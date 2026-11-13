#pragma once

/**
 * @file
 * @brief Where an entity is in the world.
 */

#include "Engine/Common/Math/Vector.hpp"

namespace Engine::Component {
    /**
     * @brief Where an entity is and how it is turned.
     *
     * For a body, position is its center of mass: the point it turns
     * around, where forces apply. Its Collider's shape turns with it.
     */
    struct Transform {
        /** @brief Position, in m. */
        Math::Vector2f position;

        /** @brief Rotation, in radians, counter-clockwise (from +X towards +Y). 0: as built. */
        float rotation = 0.f;
    };
}
