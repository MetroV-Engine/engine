/*
    Created on: 3/6/26
    Filename: Collider.hpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/


#ifndef PROTO_COLLIDER_HPP
#define PROTO_COLLIDER_HPP
#include "ComponentRegistry.hpp"
#include "Entity.hpp"
#include "IComponent.hpp"


namespace ECS::Entity {

    struct Collider : public IComponent {
        bool _canCollide;

        explicit Collider(const bool canCollide = true) : _canCollide(canCollide) {}

        std::string getName() override
        {
            return "Collider";
        }

        std::vector<ECS::Reflection::FieldDescriptor> getFields() override
        {
            return {
                { "canCollide", ECS::Reflection::FieldType::Bool, &_canCollide }
            };
        }

        void fromJson(const nlohmann::json& j) override
        {
            _canCollide = j.value("canCollide", _canCollide);
        }
    };
}

REGISTER_COMPONENT(ECS::Entity::Collider, "Collider")


#endif //PROTO_COLLIDER_HPP
