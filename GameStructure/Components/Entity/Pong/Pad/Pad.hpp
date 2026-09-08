/*
    Created on: 3/2/26
    Filename: Pad.hpp

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/


#ifndef PROTO_PAD_HPP
#define PROTO_PAD_HPP

#include <map>
#include "SdlTypes.hpp"
#include "IComponent.hpp"
#include "ComponentRegistry.hpp"
#include "Registry.hpp"
#include "System/ISystem.hpp"
#include "Entity/Base/Controlable/Controlable.hpp"
#include "Entity/Base/Position/Position.hpp"

namespace ECS::Entity
{
    struct Pad : public IComponent {
        explicit Pad(ECS::registry& reg,
                     std::vector<ECS::System::ISystem*>& sys,
                     std::map<Moves, Proto::Key>& controlsMap,
                     Position pos,
                     Proto::Color color
        );

        std::string getName() override { return "Pad"; }
        std::vector<ECS::Reflection::FieldDescriptor> getFields() override { return {}; }
        void fromJson(const nlohmann::json& j) override {}
    };
}

#endif //PROTO_PAD_HPP
