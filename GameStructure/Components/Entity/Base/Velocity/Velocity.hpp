/*
    Created on: 2/20/26
    Filename: Velocity.hpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/


#ifndef PROTO_VELOCITY_HPP
#define PROTO_VELOCITY_HPP

#include <string>
#include <vector>
#include "IComponent.hpp"
#include "ComponentRegistry.hpp"

namespace ECS::Entity {

    struct Velocity : public IComponent {
        float _vx;
        float _vy;
        float _speed;
        float _speedX;
        float _speedY;

        explicit Velocity(const float vx = 0.0f, const float vy = 0.0f, const float speed = 500.0f)
            : _vx(vx), _vy(vy), _speed(speed), _speedX(speed), _speedY(speed) {}

        std::string getName() override
        {
            return "Velocity";
        }

        std::vector<ECS::Reflection::FieldDescriptor> getFields() override
        {
            return {
                { "vx", ECS::Reflection::FieldType::Float, &_vx },
                { "vy", ECS::Reflection::FieldType::Float, &_vy },
                { "speed", ECS::Reflection::FieldType::Float, &_speed },
                { "speedX", ECS::Reflection::FieldType::Float, &_speedX },
                { "speedY", ECS::Reflection::FieldType::Float, &_speedY }
            };
        }

        void fromJson(const nlohmann::json& j) override
        {
            _vx    = j.value("vx",    _vx);
            _vy    = j.value("vy",    _vy);
            _speed = j.value("speed", _speed);
            _speedX = j.value("speedX", _speed);
            _speedY = j.value("speedY", _speed);
        }
    };

}

// Register the component
REGISTER_COMPONENT(ECS::Entity::Velocity, "Velocity")

#endif //PROTO_VELOCITY_HPP
