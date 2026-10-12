#pragma once

/**
 * @file
 * @brief Entry point of the reflection layer: include this to use E_REFLECT.
 *
 * - Descriptor.hpp: E_REFLECT / E_FIELD / E_END and compile-time visitors.
 * - TypeRegistry.hpp, TypeInfo.hpp, ContainerTraits.hpp: runtime descriptions.
 * - Ref.hpp: type-erased view on a value (fields, containers, paths).
 * - ComponentReflection.hpp: component access by TypeInfo on a Registry.
 *
 * PrettyPrintRefl.hpp is optional and included separately.
 */

#include "ComponentReflection.hpp"
#include "ContainerTraits.hpp"
#include "Descriptor.hpp"
#include "Ref.hpp"
#include "TypeInfo.hpp"
#include "TypeRegistry.hpp"
