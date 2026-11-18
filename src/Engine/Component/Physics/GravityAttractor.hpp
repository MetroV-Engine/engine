#pragma once

/**
 * @file
 * @brief A body that attracts the others by universal gravitation.
 */

#include "Engine/Component/Reflection.hpp"

namespace Engine::Component {
    /**
     * @brief Attracts every RigidBody with F = G * mA * mB / d².
     *
     * Needs a Transform on the same entity: the attraction comes from its
     * position. Its mass is independent of any RigidBody of the entity, so a
     * fixed planet needs no RigidBody, and a moving one has both.
     */
    struct GravityAttractor {
        /** @brief Attracting mass in kg. */
        float mass = 0.f;
    };
}

E_REFLECT_AS(Engine::Component::GravityAttractor, "GravityAttractor")
    E_FIELD(mass, float)
E_END
