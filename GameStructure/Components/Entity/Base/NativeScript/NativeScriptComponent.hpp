/*
    Created on: 3/10/26
    Filename: NativeScriptComponent.hpp
    Description: Native C++ script component — holds an OnUpdate lambda that runs
                 each frame. This is the Phase 4.5 bridge: proves the scripting
                 architecture works end-to-end before Mono is embedded.
                 In Phase 5, the Mono invocation replaces the lambda call.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#ifndef PROTO_NATIVESCRIPTCOMPONENT_HPP
#define PROTO_NATIVESCRIPTCOMPONENT_HPP

#include <functional>
#include <string>
#include <vector>
#include "IComponent.hpp"
#include "Entity.hpp"
#include "Registry.hpp"
#include "../../../../../Editor/Save/json.hpp"

namespace ECS::Entity {

    struct NativeScriptComponent : public IComponent {
        using ScriptFn = std::function<void(ECS::registry &, ECS::Entity::entity, float dt)>;

        std::string scriptName;  // human-readable label shown in the Toolbox
        ScriptFn    onUpdate;
        ScriptFn    onStart;
        bool        started = false;
        bool        active  = true;

        explicit NativeScriptComponent(std::string name     = "NativeScript",
                                        ScriptFn    updateFn = nullptr,
                                        ScriptFn    startFn  = nullptr)
            : scriptName(std::move(name))
            , onUpdate  (std::move(updateFn))
            , onStart   (std::move(startFn))
        {}

        std::string getName() override { return "NativeScript"; }

        // Fields shown in the Toolbox — lambdas can't be reflected,
        // so we only expose the name and active flag.
        std::vector<ECS::Reflection::FieldDescriptor> getFields() override
        {
            return {
                { "Script",  ECS::Reflection::FieldType::String, &scriptName },
                { "Active",  ECS::Reflection::FieldType::Bool,   &active     }
            };
        }

        void fromJson(const nlohmann::json& j) override
        {
            if (j.contains("scriptName")) scriptName = j["scriptName"].get<std::string>();
            if (j.contains("active"))     active     = j["active"].get<bool>();
        }
    };

} // ECS::Entity

#endif //PROTO_NATIVESCRIPTCOMPONENT_HPP

