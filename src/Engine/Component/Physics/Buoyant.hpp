#pragma once

/**
 * @file
 * @brief Makes a body float: the shape it displaces in a fluid.
 */

#include "Collider.hpp"
#include "Engine/Common/Math/Vector.hpp"

namespace Engine::Component {
    /**
     * @brief Makes a RigidBody float in FluidVolumes: BuoyancySystem
     *        measures how much of this shape is in the fluid, under its
     *        surface, and pushes the body up at the center of that part.
     *
     * Any form of a Collider: a box, a circle, a polygon, placed and turned
     * like one (offset from the Transform, turned with its rotation). To
     * float with the collision shape: Buoyant{collider.shape, collider.offset}.
     * A body without it is not affected by fluids.
     */
    struct Buoyant {
        /** @brief The displaced shape, in the body's own space: a 1 m box by default. */
        Collider::Shape shape = Collider::Box{};

        /** @brief Center of the shape relative to the Transform's position, in m (turned with it). */
        Math::Vector2f offset;

        /** @brief The shape where the body is, in world space. */
        [[nodiscard]] constexpr Collider::WorldShape worldShape(const Transform& transform) const noexcept {
            return Collider::place(shape, offset, transform);
        }
    };
}
