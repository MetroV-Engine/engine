#include <iostream>
#include <string>

#include "ECS/ECS.hpp"
#include "ECS/Reflection/Reflection.hpp"
#include "ECS/Reflection/PrettyPrint.hpp"

// Components: plain structs. E_REFLECT makes them visible to the tools.
struct Position { float x = 0; float y = 0; };
struct Velocity { float dx = 0; float dy = 0; };
struct Health { int current = 100; };

E_REFLECT(Position)
    E_FIELD(x, float)
    E_FIELD(y, float)
E_END

E_REFLECT(Health)
    E_FIELD(current, int)
E_END

// Gameplay helper with its own runDeferred, so it is safe to call anywhere,
// including from a system that is iterating a view.
void killWithLoot(ECS::Registry& world, ECS::Entity enemy) {
    world.runDeferred([&] {
        const ECS::Entity loot = world.spawnEntity("loot");
        world.addComponent(loot, world.getComponent<Position>(enemy));
        world.killEntity(enemy);
    });
}

struct DamageSystem : ECS::ISystem {
    void update(ECS::Registry& world, double) override {
        for (auto [entity, health] : world.view<Health>()) {
            health.current -= 60;
            if (health.current <= 0) {
                killWithLoot(world, entity);   // applied after this loop, not during it
            }
        }
    }
    const std::string name() const override { return "DamageSystem"; }
};

int main() {
    ECS::Registry world;

    // --- Game code: the typed API, as before ----------------------------------
    const ECS::Entity player = world.spawnEntity("player");   // named: gets an ECS::Name component
    const ECS::Entity tile = world.spawnEntity();              // unnamed: no Name, costs nothing
    world.addComponent(player, Position{1, 2});
    world.emplaceComponent<Velocity>(player, 0.5f, 0.0f);
    world.emplaceComponent<Health>(player, 50);
    world.addComponent(tile, Position{5, 5});

    for (auto [entity, position, velocity] : world.view<Position, Velocity>()) {
        position.x += velocity.dx;                             // player.x: 1 -> 1.5
    }

    // A name is a regular component: rename by editing it.
    world.getComponent<ECS::Name>(player).value = "hero";

    // --- New: checking a handle kept across frames ----------------------------
    const ECS::Entity target = player;
    // world.isAlive(target) -> true, world.entityCount() -> 2

    // --- Systems: mutations are deferred until the system returns -------------
    ECS::SystemManager systems;
    systems.addSystem<DamageSystem>(world);
    systems.update(world);                                     // player: 50 -> -10, killed, loot spawned

    std::cout << "target alive after the damage system: " << std::boolalpha << world.isAlive(target)
              << " (" << world.entityCount() << " entities)\n\n";

    // --- Tools: what an editor sees through the inspector ---------------------
    auto inspector = world.inspect();
    const ECS::EntityList entities = inspector.entities();    // keep it: always up to date

    auto printWorld = [&] {
        for (const ECS::Entity entity : entities) {
            std::cout << inspector.displayName(entity) << "\n";   // its Name, or "Entity <index>"
            for (const ECS::reflect::Ref component : inspector.list(entity)) {
                std::cout << "  " << ECS::reflect::toPrettyString(component, {.indent = 0}) << "\n";
            }
        }
    };

    printWorld();

    world.spawnEntity("spawned later");
    std::cout << "\nSame EntityList after one more spawn:\n";
    printWorld();
}
