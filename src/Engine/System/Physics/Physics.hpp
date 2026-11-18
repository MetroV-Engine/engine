#pragma once

/**
 * @file
 * @brief The single entry point of the physics: every system, in its order.
 */

#include "ECS/ECS.hpp"
#include "Engine/System/Physics/Buoyancy/BuoyancySystem.hpp"
#include "Engine/System/Physics/Character/CharacterControllerSystem.hpp"
#include "Engine/System/Physics/Collision/CollisionResponseSystem.hpp"
#include "Engine/System/Physics/Collision/CollisionSystem.hpp"
#include "Engine/System/Physics/Drag/DragSystem.hpp"
#include "Engine/System/Physics/Friction/FrictionSystem.hpp"
#include "Engine/System/Physics/Gravity/GravitySystem.hpp"
#include "Engine/System/Physics/Integration/IntegrationSystem.hpp"
#include "Engine/System/Physics/Radial/RadialFieldSystem.hpp"
#include "Engine/System/Physics/Spring/SpringSystem.hpp"

namespace Engine::Physics {
    /**
     * @brief The step the physics is tuned for, in s: 1/60. Call
     *        SystemManager::update with it, a fixed step: jump heights,
     *        resting speeds and the stability of springs depend on it.
     */
    inline constexpr double Step = 1.0 / 60.0;

    /**
     * @brief Registers every physics system in the order the physics needs:
     *
     * 1. forces: GravitySystem, SpringSystem, RadialFieldSystem, DragSystem,
     *    BuoyancySystem (they only add to RigidBody::force and torque);
     * 2. CharacterControllerSystem, after the forces it compensates;
     * 3. FrictionSystem, after every other force (static friction holds
     *    against all of them) and after the controller (whose grip it sees);
     * 4. IntegrationSystem: forces into velocities, velocities into motion;
     * 5. CollisionSystem, where the bodies ended up;
     * 6. CollisionResponseSystem: separation, bounces, impact friction, and
     *    the SurfaceContacts the next step's friction and characters read.
     *
     * SystemManager runs systems in the order they were added: add the
     * game's systems that write inputs and forces (CharacterInput, a
     * RigidBody's force) before calling this, and those that read the
     * results (cameras, gameplay reading Contacts) after. Call it once per
     * SystemManager. Each system sets the resources it needs
     * (GravitySettings, CollisionSettings, Contacts) when they are not
     * there yet: set your own before to change them.
     */
    inline void install(ECS::Registry& world, ECS::SystemManager& systems) {
        systems.addSystem<System::GravitySystem>(world);
        systems.addSystem<System::SpringSystem>(world);
        systems.addSystem<System::RadialFieldSystem>(world);
        systems.addSystem<System::DragSystem>(world);
        systems.addSystem<System::BuoyancySystem>(world);
        systems.addSystem<System::CharacterControllerSystem>(world);
        systems.addSystem<System::FrictionSystem>(world);
        systems.addSystem<System::IntegrationSystem>(world);
        systems.addSystem<System::CollisionSystem>(world);
        systems.addSystem<System::CollisionResponseSystem>(world);
    }
}
