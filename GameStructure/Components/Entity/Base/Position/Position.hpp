/*
    Created on: 2/20/26
    Filename: Position.hpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/


#ifndef PROTO_POSITION_HPP
#define PROTO_POSITION_HPP

#include <string>
#include <vector>
#include "IComponent.hpp"
#include "ComponentRegistry.hpp"

namespace ECS::Entity {
    struct Position : public IComponent {
        float _x;
        float _y;

        explicit Position(const float x = 100.0f, const float y = 100.0f)
            : _x(x), _y(y)
        {
        }

        std::string getName() override
        {
            return "Position";
        }

        std::vector<ECS::Reflection::FieldDescriptor> getFields() override
        {
            return {
                { "x", ECS::Reflection::FieldType::Float, &_x },
                { "y", ECS::Reflection::FieldType::Float, &_y }
            };
        }

        void fromJson(const nlohmann::json& j) override
        {
            _x = j.value("x", _x);
            _y = j.value("y", _y);
        }
    };
}

// Register the component
REGISTER_COMPONENT(ECS::Entity::Position, "Position")

#endif //PROTO_POSITION_HPP
