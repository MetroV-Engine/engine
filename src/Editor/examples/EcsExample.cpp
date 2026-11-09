/*
    Created on: 03/11/2026
    Filename: EcsExample.cpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

// A small game opening the editor on its own ECS world. This is all game
// code needs to do: reflect its components, build its world and systems, and
// give them to the Editor.
//
//   ./build/gui_ecs_example
//
// Select an entity in the Hierarchy to edit its components in the Inspector;
// Play (F5) runs the systems, Stop (Shift+F5) puts the world back.

#include "core/Editor/Editor.hpp"
#include "core/GuiContext/GuiContext.hpp"
#include "core/Logger/Logger.hpp"

#include "ECS/ECS.hpp"
#include "ECS/Reflection/Reflection.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <exception>
#include <string>
#include <vector>

// ---- Components: plain structs, reflected so the editor can show them

// Two float fields x, y: a Vec2 in the Inspector. r, g, b, a: a Color.
struct Vector2
{
    float x = 0.0f;
    float y = 0.0f;
};

struct Rgba
{
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
    float a = 1.0f;
};

enum class Team
{
    Neutral,
    Player,
    Enemy,
};

struct Transform
{
    Vector2 position;
    float rotation = 0.0f;
};

struct Velocity
{
    Vector2 value;
};

struct Body
{
    float radius = 12.0f;
    Rgba color;
    bool visible = true;
};

struct Health
{
    int current = 100;
    int max = 100;
};

struct Character
{
    std::string title;
    Team team = Team::Neutral;
    std::uint8_t level = 1; // Clamped to 0..255 by the Inspector
    std::vector<std::string> tags; // Shown, not editable yet
};

// Any other struct: a group of fields in the Inspector.
struct Range
{
    float min = 0.0f;
    float max = 1.0f;
};

// Makes the Body's radius oscillate within radius.
struct Pulse
{
    Range radius{8.0f, 16.0f};
    float speed = 2.0f;
    double phase = 0.0;
};

E_REFLECT(Vector2)
    E_FIELD(x, float)
    E_FIELD(y, float)
E_END

E_REFLECT(Rgba)
    E_FIELD(r, float)
    E_FIELD(g, float)
    E_FIELD(b, float)
    E_FIELD(a, float)
E_END

E_REFLECT(Transform)
    E_FIELD(position, Vector2)
    E_FIELD(rotation, float)
E_END

E_REFLECT(Velocity)
    E_FIELD(value, Vector2)
E_END

E_REFLECT(Body)
    E_FIELD(radius, float)
    E_FIELD(color, Rgba)
    E_FIELD(visible, bool)
E_END

E_REFLECT(Health)
    E_FIELD(current, int)
    E_FIELD(max, int)
E_END

E_REFLECT(Character)
    E_FIELD(title, std::string)
    E_FIELD(team, Team)
    E_FIELD(level, std::uint8_t)
    E_FIELD(tags, std::vector<std::string>)
E_END

E_REFLECT(Range)
    E_FIELD(min, float)
    E_FIELD(max, float)
E_END

E_REFLECT(Pulse)
    E_FIELD(radius, Range)
    E_FIELD(speed, float)
    E_FIELD(phase, double)
E_END

// ---- Resource and systems

// Size of the play area, in world units. A resource: not shown by the editor.
struct Arena
{
    float width = 640.0f;
    float height = 360.0f;
};

// Moves what has a Velocity, bouncing on the borders of the arena.
class MovementSystem : public ECS::ISystem
{
public:
    void update(ECS::Registry &world, double dt) override
    {
        const Arena &arena = world.getResource<Arena>();
        const float step = static_cast<float>(dt);
        for (auto [entity, transform, velocity] : world.view<Transform, Velocity>()) {
            Vector2 &position = transform.position;
            position.x += velocity.value.x * step;
            position.y += velocity.value.y * step;
            if (position.x < 0.0f || position.x > arena.width)
                velocity.value.x = -velocity.value.x;
            if (position.y < 0.0f || position.y > arena.height)
                velocity.value.y = -velocity.value.y;
            position.x = std::clamp(position.x, 0.0f, arena.width);
            position.y = std::clamp(position.y, 0.0f, arena.height);
        }
    }

    const std::string name() const override { return "MovementSystem"; }
};

class PulseSystem : public ECS::ISystem
{
public:
    void update(ECS::Registry &world, double dt) override
    {
        for (auto [entity, pulse, body] : world.view<Pulse, Body>()) {
            pulse.phase += dt * pulse.speed;
            const float t = static_cast<float>(std::sin(pulse.phase) + 1.0) * 0.5f;
            body.radius = pulse.radius.min + (pulse.radius.max - pulse.radius.min) * t;
        }
    }

    const std::string name() const override { return "PulseSystem"; }
};

// ---- Level

void populate(ECS::Registry &world)
{
    const ECS::Entity player = world.spawnEntity("Player");
    world.addComponent(player, Transform{{120.0f, 200.0f}});
    world.addComponent(player, Velocity{{90.0f, 40.0f}});
    world.addComponent(player, Body{18.0f, {0.3f, 0.8f, 0.4f, 1.0f}});
    world.addComponent(player, Health{100, 100});
    world.addComponent(player, Character{"Hero", Team::Player, 3, {"hero", "controllable"}});

    for (int i = 1; i <= 3; i++) {
        const ECS::Entity slime = world.spawnEntity("Slime " + std::to_string(i));
        world.addComponent(slime, Transform{{150.0f * static_cast<float>(i) + 60.0f, 90.0f}});
        world.addComponent(slime, Velocity{{-50.0f * static_cast<float>(i), 70.0f}});
        world.addComponent(slime, Body{12.0f, {0.9f, 0.3f, 0.3f, 1.0f}});
        world.addComponent(slime, Health{30, 30});
        world.addComponent(slime, Character{"Slime", Team::Enemy, 1, {"enemy"}});
        world.addComponent(slime, Pulse{{10.0f, 14.0f}, 3.0f + static_cast<float>(i)});
    }

    const ECS::Entity coin = world.spawnEntity("Coin");
    world.addComponent(coin, Transform{{320.0f, 300.0f}});
    world.addComponent(coin, Body{6.0f, {1.0f, 0.85f, 0.2f, 1.0f}});
    world.addComponent(coin, Pulse{{5.0f, 8.0f}, 4.0f});

    // Most entities have no name: the Hierarchy shows them as "Entity <index>".
    const ECS::Entity rock = world.spawnEntity();
    world.addComponent(rock, Transform{{520.0f, 260.0f}});
    world.addComponent(rock, Body{20.0f, {0.5f, 0.5f, 0.55f, 1.0f}});
}

// Draws the arena and the bodies in the Viewport, fitted to it, until the
// engine renders there.
void drawWorld(ECS::Registry &world, ImDrawList *drawList, ImVec2 min, ImVec2 max)
{
    const Arena &arena = world.getResource<Arena>();
    const float scale = std::min((max.x - min.x) / arena.width, (max.y - min.y) / arena.height) * 0.9f;
    const ImVec2 origin(min.x + ((max.x - min.x) - arena.width * scale) * 0.5f,
        min.y + ((max.y - min.y) - arena.height * scale) * 0.5f);
    drawList->AddRect(origin, ImVec2(origin.x + arena.width * scale, origin.y + arena.height * scale),
        IM_COL32(128, 128, 128, 255));

    for (auto [entity, transform, body] : world.view<Transform, Body>()) {
        if (!body.visible)
            continue;
        const ImVec2 center(origin.x + transform.position.x * scale, origin.y + transform.position.y * scale);
        const Rgba &c = body.color;
        drawList->AddCircleFilled(center, std::max(body.radius * scale, 1.0f), ImGui::ColorConvertFloat4ToU32(ImVec4(c.r, c.g, c.b, c.a)));
    }
}

int main()
{
    Logger &logger = Logger::instance();
    logger.captureSdlLogs();
    try {
        // Declared before the editor, so it outlives it.
        ECS::Registry world;
        ECS::SystemManager systems;
        world.setResource<Arena>();
        systems.addSystem<MovementSystem>(world);
        systems.addSystem<PulseSystem>(world);
        populate(world);

        GuiContext gui("MetroV Editor - ECS example", 1600, 900);
        Editor editor(gui, world, systems);
        editor.setViewportRenderer(
            [&world](ImDrawList *drawList, ImVec2 min, ImVec2 max) { drawWorld(world, drawList, min, max); });
        // Dragged from the Entity Palette onto the Hierarchy or the Viewport.
        editor.addEntityTemplate({"Ball", "Ball", {"Transform", "Velocity", "Body"}});
        editor.run();
    } catch (const std::exception &e) {
        logger.error("Fatal", e.what());
        return 1;
    }
    return 0;
}
