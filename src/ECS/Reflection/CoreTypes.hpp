#pragma once

/**
 * @file
 * @brief Reflection of the components defined by the ECS core.
 *
 * The core does not depend on the reflection layer, so its components are
 * reflected here. Included by Reflection.hpp: any program using reflection
 * sees them in RegistryInspector::types() and inspector.list(entity).
 */

#include <string>

#include "../Core/Entity/Name.hpp"
#include "Descriptor.hpp"
#include "TypeRegistry.hpp"   // defines the registration E_REFLECT calls

E_REFLECT(ECS::Name)
    E_FIELD(value, std::string)
E_END
