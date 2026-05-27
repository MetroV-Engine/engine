/*
    Created on: 2/19/26
    Filename: IComponent.hpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/


#ifndef PROTO_ICOMPONENT_HPP
#define PROTO_ICOMPONENT_HPP

#include <string>
#include <vector>
#include "Reflection/FieldDescriptor.hpp"
#include "../../Editor/Save/json.hpp"

class IComponent {
    public:
        virtual ~IComponent() = default;

        virtual std::string getName() = 0;

        // Returns a list of reflected fields for generic Toolbox rendering.
        // Each entry holds the field name, its type tag, and a raw pointer into this component's data.
        virtual std::vector<ECS::Reflection::FieldDescriptor> getFields() = 0;

        // Serialize this component to JSON. The default implementation uses reflection to serialize all fields.
        virtual void fromJson(const nlohmann::json& j) = 0;
};

#endif //PROTO_ICOMPONENT_HPP

