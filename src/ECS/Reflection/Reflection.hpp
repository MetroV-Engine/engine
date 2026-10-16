#pragma once

/**
 * @file
 * @brief Entry point of the reflection layer: include this to use E_REFLECT.
 *
 * - Descriptor.hpp: E_REFLECT / E_FIELD / E_END and compile-time visitors.
 * - TypeInfo.hpp, TypeRegistry.hpp, ContainerTraits.hpp: runtime type descriptions.
 * - Ref.hpp: type-erased view on a value (fields, containers, paths).
 * - RegistryInspector.hpp: component access on a Registry from a TypeInfo,
 *   the entry point of the tools (editor, scripts, scenes).
 * - CoreTypes.hpp: reflection of the core's own components (ECS::Name).
 * - detail/: internals, not meant to be included directly.
 *
 * PrettyPrint.hpp is optional and included separately.
 */

#include "ContainerTraits.hpp"
#include "CoreTypes.hpp"
#include "Descriptor.hpp"
#include "Ref.hpp"
#include "RegistryInspector.hpp"
#include "TypeInfo.hpp"
#include "TypeRegistry.hpp"
