#pragma once

/**
 * @file
 * @brief Entry point of the ECS core for game code: include this.
 *
 * - Core/Registry.hpp: the world (entities, components, resources, views).
 * - Core/System/: ISystem and SystemManager.
 *
 * The reflection layer (E_REFLECT, world.inspect()) is separate: include
 * ECS/Reflection/Reflection.hpp where it is needed.
 */

#include "Core/Registry.hpp"
#include "Core/System/ISystem.hpp"
#include "Core/System/SystemManager.hpp"
