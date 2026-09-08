/*
    Created on: 3/2/26
    Filename: Ball.hpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/


#ifndef PROTO_BALL_HPP
#define PROTO_BALL_HPP
#include <string>
#include <vector>

#include "IComponent.hpp"
#include "ComponentRegistry.hpp"
#include "System/ISystem.hpp"

namespace ECS::Entity {
    struct BallTag : public IComponent {
        std::string getName() override
        {
            return "BallTag";
        }

        std::vector<ECS::Reflection::FieldDescriptor> getFields() override
        {
            return {};
        }

        void fromJson(const nlohmann::json& j) override
        {
            // No fields to deserialize for BallTag
        }
    };

    struct Ball : public  IComponent {
        explicit Ball(registry &reg, std::vector<System::ISystem *> &sys);

        std::string getName() override
        {
            return "Ball";
        }

        std::vector<ECS::Reflection::FieldDescriptor> getFields() override
        {
            return {};
        }

        void fromJson(const nlohmann::json& j) override
        {
            // No fields to deserialize for Ball
        }
    };
}

// Note: Ball components are not registered as they require complex initialization
REGISTER_COMPONENT(ECS::Entity::BallTag, "BallTag")
// REGISTER_COMPONENT(ECS::Entity::Ball, "Ball")

#endif //PROTO_BALL_HPP
