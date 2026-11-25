#pragma once

/**
 * @file
 * @brief Entry point of the ECS core for game code: include this.
 *
 * - Core/Registry.hpp: the world (entities, components, resources, views).
 * - Core/System/: ISystem, SystemManager, and SystemCatalog (systems created
 *   by name, registered with E_SYSTEM).
 *
 * The reflection layer (E_REFLECT, world.inspect()) is separate: include
 * ECS/Reflection/Reflection.hpp where it is needed.
 */

#include "Core/Registry.hpp"
#include "Core/System/ISystem.hpp"
#include "Core/System/SystemCatalog.hpp"
#include "Core/System/SystemManager.hpp"
