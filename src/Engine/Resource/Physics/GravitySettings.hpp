#pragma once

/**
 * @file
 * @brief World-wide gravity parameters, stored as an ECS resource.
 */

#include "ECS/ECS.hpp"
#include "Engine/Common/Math/Vector.hpp"
#include "Engine/Common/Physic/Forces/Gravity.hpp"

namespace Engine::Resource {
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

        /**
         * @brief The world's GravitySettings, or the defaults when it has
         *        none (GravitySystem sets them in init(), but a world without
         *        GravitySystem still has a gravity to float or fall with).
         *
         * A copy: cheap (three floats), and valid even without the resource.
         * To change the world's gravity, use world.getResource<GravitySettings>().
         */
        [[nodiscard]] static GravitySettings of(const ECS::Registry& world) {
            return world.hasResource<GravitySettings>() ? world.getResource<GravitySettings>() : GravitySettings{};
        }
    };
}
