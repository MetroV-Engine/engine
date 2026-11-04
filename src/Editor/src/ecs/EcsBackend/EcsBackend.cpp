/*
    Created on: 03/11/2026
    Filename: EcsBackend.cpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "EcsBackend.hpp"
#include "core/CommandRegistry/CommandRegistry.hpp"
#include "core/Logger/Logger.hpp"

#include <algorithm>
#include <string>
#include <vector>

namespace reflect = ECS::reflect;

EcsBackend::EcsBackend(ECS::Registry &world, ECS::SystemManager &systems)
    : _scene(world), _simulation(world, systems), _systems(systems)
{
    // Registration runs before main, where it cannot throw: its failures
    // are only recorded.
    for (const std::string &error : reflect::RegistryInspector::errors())
        Logger::instance().error("Reflection", error);
}

void EcsBackend::registerCommands(CommandRegistry &commands)
{
    commands.add("components", "components", "List the component types (reflected with E_REFLECT)", [](const auto &) {
        std::vector<std::string> names;
        for (const reflect::TypeInfo *type : reflect::RegistryInspector::types())
            names.push_back(type->name + (type->construct ? "" : " (no default constructor: cannot be added)"));
        std::ranges::sort(names);
        Logger &logger = Logger::instance();
        logger.info("Console", std::to_string(names.size()) + " component types");
        for (const std::string &name : names)
            logger.info("Console", "  " + name);
    });
}

std::string EcsBackend::description() const
{
    const std::size_t entities = _scene.world().entityCount();
    const std::size_t types = reflect::RegistryInspector::types().size();
    const std::size_t systems = _systems.size();
    return "Editing an ECS world: " + std::to_string(entities) + (entities == 1 ? " entity, " : " entities, ")
        + std::to_string(types) + " component type" + (types == 1 ? ", " : "s, ") + std::to_string(systems)
        + (systems == 1 ? " system" : " systems");
}
