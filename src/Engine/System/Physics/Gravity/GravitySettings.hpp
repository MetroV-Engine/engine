#pragma once

/**
 * @file
 * @brief World-wide gravity parameters, stored as an ECS resource.
 */

#include "Engine/Common/Math/Vector.hpp"
#include "Engine/Common/Physic/Forces/Gravity.hpp"

namespace Engine::System {
    /**
     * @brief Gravity of the world, read by GravitySystem every step.
     *
     * Change it at runtime with world.getResource<GravitySettings>(): low
     * gravity zones, upside-down rooms, a G strong enough for game-scale
     * attractors.
     */
    struct GravitySettings {
        /**
         * @brief Uniform gravity, in m/s², pointing "down".
         *
         * The default assumes Y points up; set (0, +g) for a Y-down world.
         */
        Math::Vector2f gravity{0.f, -Physic::StandardGravity<float>};

        /**
         * @brief Constant G used by GravityAttractor, in N·m²/kg².
         *
         * The real G makes game-scale attractors (a few tons, a few meters)
         * far too weak to notice: raise it to tune them.
         */
        float gravitationalConstant = Physic::GravitationalConstant<float>;
    };
}
