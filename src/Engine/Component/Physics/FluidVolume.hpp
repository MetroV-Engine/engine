#pragma once

/**
 * @file
 * @brief A body of water (or any fluid): a box whose top is its surface.
 */

#include "Engine/Common/Math/Vector.hpp"

namespace Engine::Component {
    /**
     * @brief An axis-aligned box of fluid, centered on the entity's
     *        Transform. Its top side is the surface.
     *
     * BuoyancySystem pushes every Buoyant body in it up by the weight of
     * the fluid it displaces (Archimedes), and slows it with `drag`. The
     * surface is the top side when Y points up, as GravitySettings assumes
     * by default.
     *
     * Densities are per area (kg/m²), as if every body were 1 m deep: a
     * body floats when its mass / area is below `density`.
     */
    struct FluidVolume {
        /** @brief Half of the box's width and height, in m. */
        Math::Vector2f halfSize{1.f, 1.f};

        /** @brief Density rho of the fluid, in kg/m² (2D), >= 0. */
        float density = 1.f;

        /**
         * @brief Linear drag in the fluid, in N·s/m per m² under the
         *        surface, >= 0: without it, a floating body bobs forever.
         */
        float drag = 0.f;
    };
}
