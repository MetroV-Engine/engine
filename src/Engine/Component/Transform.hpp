#pragma once

/**
 * @file
 * @brief Where an entity is in the world.
 */

#include "Engine/Common/Math/Vector.hpp"

namespace Engine::Component {
    /**
     * @brief Position of an entity, in meters.
     *
     * Only the position for now; rotation and scale come with the systems
     * that need them.
     */
    struct Transform {
        Math::Vector2f position;
    };
}
