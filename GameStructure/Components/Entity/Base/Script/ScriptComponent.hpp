/*
    Created on: 3/9/26
    Filename: ScriptComponent.hpp
    Description: Script component — stores the path and class info for a C# script
                 attached to an entity. Data-only for Phase 1 (no execution yet).

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#ifndef PROTO_SCRIPTCOMPONENT_HPP
#define PROTO_SCRIPTCOMPONENT_HPP

#include <string>
#include <vector>
#include "IComponent.hpp"
#include "ComponentRegistry.hpp"

namespace ECS::Entity {

    struct ScriptComponent : public IComponent {
        std::string scriptPath;      // path to the .cs file   e.g. "scripts/Player.cs"
        std::string namespaceName;   // C# namespace            e.g. "Metrovy"
        std::string className;       // C# class name           e.g. "Player"
        bool        active = true;

        explicit ScriptComponent(std::string path = "",
                                  std::string cls  = "",
                                  std::string ns   = "Metrovy")
            : scriptPath    (std::move(path))
            , namespaceName (std::move(ns))
            , className     (std::move(cls))
        {}

        std::string getName() override
        {
            return "Script";
        }

        std::vector<ECS::Reflection::FieldDescriptor> getFields() override
        {
            return {
                { "Script Path", ECS::Reflection::FieldType::String, &scriptPath    },
                { "Namespace",   ECS::Reflection::FieldType::String, &namespaceName },
                { "Class Name",  ECS::Reflection::FieldType::String, &className     },
                { "Active",      ECS::Reflection::FieldType::Bool,   &active        }
            };
        }

        void fromJson(const nlohmann::json& j) override
        {
            if (j.contains("scriptPath"))   scriptPath    = j["scriptPath"].get<std::string>();
            if (j.contains("namespace"))    namespaceName = j["namespace"].get<std::string>();
            if (j.contains("className"))    className     = j["className"].get<std::string>();
            if (j.contains("active"))       active        = j["active"].get<bool>();
        }
    };

} // ECS::Entity

REGISTER_COMPONENT(ECS::Entity::ScriptComponent, "Script")

#endif //PROTO_SCRIPTCOMPONENT_HPP

