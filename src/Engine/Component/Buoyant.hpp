#pragma once

/**
 * @file
 * @brief The shape a body displaces in a fluid.
 */

#include "Engine/Common/Math/Vector.hpp"

namespace Engine::Component {
    /**
     * @brief Makes a RigidBody float in FluidVolumes: BuoyancySystem
     *        measures how much of this box, centered on the entity's
     *        Transform, is under the surface.
     *
     * A body without it is not affected by fluids.
     */
    struct Buoyant {
        /** @brief Half of the body's width and height, in m. */
        Math::Vector2f halfSize{0.5f, 0.5f};
    };
}
