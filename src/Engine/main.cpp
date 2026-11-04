/*
    Created on: 04/11/2026
    Filename: main.cpp
    Description: Minimal engine entry point: opens the editor on an ECS world

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "core/Editor/Editor.hpp"
#include "core/GuiContext/GuiContext.hpp"
#include "core/Logger/Logger.hpp"

#include "ECS/ECS.hpp"
#include "ECS/Reflection/PrettyPrint.hpp"
#include "ECS/Reflection/Reflection.hpp"

// Only the components used here: the editor offers every component of
// Components/ anyway (see CMakeLists.txt).
#include "Components/Position.hpp"
#include "Components/Velocity.hpp"

#include <exception>
#include <string>

// ---- System (runs when Play is pressed in the editor)

class MoveSystem : public ECS::ISystem
{
public:
    void update(ECS::Registry &world, double dt) override
    {
        const float step = static_cast<float>(dt);
        for (auto [entity, position, velocity] : world.view<Position, Velocity>()) {
            position.x += velocity.x * step;
            position.y += velocity.y * step;
        }
    }

    const std::string name() const override { return "MoveSystem"; }
};

// Logs every entity and its components, read from the ECS itself, not through
// the editor: logged before and after the editor, it shows what the editor
// really changed in the world.
void logWorld(ECS::Registry &world, const std::string &when)
{
    Logger &logger = Logger::instance();
    ECS::reflect::RegistryInspector inspector = world.inspect();
    logger.info("World", when + ": " + std::to_string(world.entityCount()) + " entities");
    for (const ECS::Entity entity : inspector.entities()) {
        std::string line = "  " + inspector.displayName(entity) + ":";
        for (const ECS::reflect::Ref component : inspector.list(entity))
            line += " " + ECS::reflect::toPrettyString(component, {.indent = 0});
        logger.info("World", line);
    }
}

int main()
{
    Logger &logger = Logger::instance();
    logger.captureSdlLogs();

    try {
        // ECS side: the world and its systems, declared before the editor so they outlive it.
        ECS::Registry world;
        ECS::SystemManager systems;
        systems.addSystem<MoveSystem>(world);

        const ECS::Entity player = world.spawnEntity("Player");
        world.addComponent(player, Position{100.0f, 100.0f});
        world.addComponent(player, Velocity{20.0f, 10.0f});
        const ECS::Entity rock = world.spawnEntity("Rock");
        world.addComponent(rock, Position{300.0f, 200.0f});

        logWorld(world, "Before the editor");

        // Editor side: the Hierarchy lists the world's entities, the Inspector
        // edits their components, Play (F5) runs the systems, Stop restores the world.
        {
            GuiContext gui("MetroV Engine", 1600, 900);
            Editor editor(gui, world, systems);
            editor.run();
        } // The editor stops the game when destroyed: the world is left as it was edited.

        logWorld(world, "After the editor");
    } catch (const std::exception &e) {
        logger.error("Fatal", e.what());
        return 1;
    }
    return 0;
}
