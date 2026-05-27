/*
    Created on: 3/5/26
    Filename: ComponentsInit.cpp
    Description: Component registration initialization - ensures all components are registered at startup

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "ComponentRegistry.hpp"

// Include all component headers to trigger their registration
#include "Entity/Base/Position/Position.hpp"
#include "Entity/Base/Velocity/Velocity.hpp"
#include "Entity/Base/Bounds/Bounds.hpp"
#include "Entity/Base/Controlable/Controlable.hpp"
#include "Entity/Base/Script/ScriptComponent.hpp"

namespace ECS::Component {
    // This function ensures the static library is linked and all components are registered
    void initializeComponents() {
        // This function doesn't need to do anything - just including the headers
        // above is enough to trigger the static registration of all components
        // via the REGISTER_COMPONENT macros in each header file
    }
}

