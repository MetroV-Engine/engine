/*
    Created on: 3/2/26
    Filename: Controlable.hpp

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/


#ifndef PROTO_CONTROLABLE_HPP
#define PROTO_CONTROLABLE_HPP

#include <map>
#include <unordered_map>
#include <vector>
#include <string>
#include "SdlTypes.hpp"
#include "IComponent.hpp"
#include "ComponentRegistry.hpp"
#include "../../../../../ECS/Component/Entity/Moves.hpp"
#include "Reflection/FieldDescriptor.hpp"

namespace ECS::Entity {

    struct Controlable : public IComponent {
        bool _canMove;
        std::map<Moves, Proto::Key> _controlsMap;

        explicit Controlable(const bool canMove = true, const std::map<Moves, Proto::Key> &controlsMap = {
                {UP, Proto::Keys::W},
                {DOWN, Proto::Keys::S},
                {LEFT, Proto::Keys::A},
                {RIGHT, Proto::Keys::D}
            })
            : _canMove(canMove), _controlsMap(controlsMap) {}

        std::string getName() override { return "Controlable"; }

        std::vector<ECS::Reflection::FieldDescriptor> getFields() override
        {
            return {
                { "canMove", ECS::Reflection::FieldType::Bool, &_canMove },
                {"layout", ECS::Reflection::FieldType::KeyMapLayout, &_controlsMap }
            };
        }

        void fromJson(const nlohmann::json& j) override
        {
            _canMove = j.value("canMove", _canMove);
            if (j.contains("layout")) {
                const auto& layout = j["layout"];
                static const std::unordered_map<std::string, Moves> moveNames = {
                    {"UP", UP}, {"DOWN", DOWN}, {"LEFT", LEFT}, {"RIGHT", RIGHT}
                };
                for (auto& [key, val] : layout.items()) {
                    auto it = moveNames.find(key);
                    if (it != moveNames.end())
                        _controlsMap[it->second] = static_cast<Proto::Key>(val.get<int>());
                }
            }
        }
    };
}

REGISTER_COMPONENT(ECS::Entity::Controlable, "Controlable")

#endif //PROTO_CONTROLABLE_HPP
