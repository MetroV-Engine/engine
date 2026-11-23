# ECS

The engine's Entity Component System. This page explains what an ECS is and
how to use this one. How each class works inside is documented in the headers,
as docstrings.

## Contents

1. [What is an ECS?](#what-is-an-ecs)
2. [Using the ECS](#using-the-ecs)
   1. [Entities](#entities)
   2. [Components](#components)
   3. [Reacting to a removed component](#reacting-to-a-removed-component)
   4. [Views: finding entities by their components](#views-finding-entities-by-their-components)
   5. [Systems](#systems)
   6. [Changing the world during a loop](#changing-the-world-during-a-loop)
   7. [Resources](#resources)
3. [Full example](#full-example)
4. [Rules to remember](#rules-to-remember)
5. [Going further](#going-further)

## What is an ECS?

An ECS splits a game object into three ideas:

| | What it is | Example |
| --- | --- | --- |
| **Entity** | An identifier, nothing more. It holds no data and no behaviour. | the player, a bullet, a tile |
| **Component** | A plain struct of data attached to an entity. | `Position`, `Velocity`, `Health` |
| **System** | The logic. It runs every frame on the entities that have the components it needs. | `MovementSystem` moves everything that has a `Position` and a `Velocity` |

```text
Entity "player"            MovementSystem: every entity with Position + Velocity
  ├── Position   ◄──────── reads Velocity, updates Position
  ├── Velocity   ◄────────┘
  └── Health
```

Why it helps:

- **Composition instead of inheritance**: a flying enemy is an entity with
  `Position`, `Velocity`, `Health` and `Flying`. No class hierarchy to design.
- **Behaviour follows data**: give an entity a `Velocity` and the movement
  system starts moving it; remove it and it stops.
- **Speed**: each component type is stored in its own contiguous array, so a
  system walks memory in order.

In this engine, the **`ECS::Registry`** is the world: it owns the entities,
their components and the resources. Everything goes through it.

## Using the ECS

Game code includes a single header:

```cpp
#include "ECS/ECS.hpp"

ECS::Registry world;
```

Components are plain structs. They need no base class and no registration:

```cpp
struct Position { float x = 0; float y = 0; };
struct Velocity { float dx = 0; float dy = 0; };
struct Health { int current = 100; int max = 100; };
```

### Entities

```cpp
ECS::Entity player = world.spawnEntity("player");   // named: gets an ECS::Name component
ECS::Entity bullet = world.spawnEntity();           // unnamed: most entities don't need a name

world.killEntity(bullet);                           // destroys it and all its components

world.isAlive(bullet);    // false: check this for a handle kept across frames (a target, a parent)
world.entityCount();      // 1
```

An `Entity` is a small handle (an index and a generation). When an entity is
killed, its index is reused later with a new generation, so an old handle is
never mistaken for the new entity.

### Components

```cpp
world.addComponent(player, Position{0, 0});         // from a value
world.emplaceComponent<Health>(player, 100, 100);   // from constructor arguments

Health& health = world.getComponent<Health>(player);
health.current -= 10;

if (world.hasComponents<Position, Velocity>(player)) { /* ... */ }

world.removeComponent<Velocity>(player);

// The name is a regular component
world.getComponent<ECS::Name>(player).value = "hero";
```

Adding a component the entity already has replaces it. `getComponent` throws
`std::out_of_range` when the entity does not have the component.

### Reacting to a removed component

Some components hold something that lives outside the ECS: a texture, a sound,
a physics body. When the component goes away, that thing must be freed too, or
it leaks.

`onRemove<T>` solves this. You give it a function once, and the ECS calls it
every time a `T` is about to be destroyed, with that `T` as argument:

```cpp
struct Sprite { TextureHandle texture; };

// Once, at startup: "when a Sprite is removed, free its texture"
world.onRemove<Sprite>([&textures](Sprite& sprite) {
    textures.release(sprite.texture);
});

// Later, both of these free the texture automatically
world.removeComponent<Sprite>(player);   // removes the Sprite: callback called
world.killEntity(enemy);                 // kills the entity, so its Sprite too: callback called
```

Good to know:

| Situation | Is the callback called? |
| --- | --- |
| `removeComponent<T>` | yes |
| `killEntity` on an entity that has a `T` | yes |
| `addComponent` on an entity that already has a `T` (replacement) | no |

- **One callback per type.** Calling `onRemove<Sprite>` a second time replaces
  the first callback; it does not add a second one.
- **Errors are ignored.** If the callback throws, the exception is swallowed and
  the component is still removed.
- **Only clean up, don't change the world.** Inside the callback, don't spawn or
  kill entities, and don't add or remove components.

### Views: finding entities by their components

A view visits every entity that has **all** the requested components, and gives
the entity plus a reference to each component:

```cpp
for (auto [entity, position, velocity] : world.view<Position, Velocity>()) {
    position.x += velocity.dx;
    position.y += velocity.dy;
}
```

The cost depends on the smallest of the requested component arrays, not on the
total number of entities. A view is a standard range, so `<ranges>` algorithms
work on it:

```cpp
auto wounded = std::ranges::count_if(world.view<Health>(), [](const auto& row) {
    return std::get<1>(row).current < std::get<1>(row).max;
});
```

### Systems

A system implements `ECS::ISystem`. A `SystemManager` owns the systems and runs
them in the order they were added:

```cpp
class MovementSystem : public ECS::ISystem {
    public:
        void update(ECS::Registry& world, double dt) override {
            for (auto [entity, position, velocity] : world.view<Position, Velocity>()) {
                position.x += velocity.dx * dt;
                position.y += velocity.dy * dt;
            }
        }

        const std::string name() const override { return "MovementSystem"; }
};

ECS::SystemManager systems;
systems.addSystem<MovementSystem>(world);   // calls its optional init(world)

// every frame
systems.update(world, dt);
```

`init(world)` and `shutdown(world)` are optional hooks, called by `addSystem`
and `removeSystem<T>(world)`.

#### Creating a system from its name

A scene file lists the systems it runs by name, and the editor lets the user
pick them. For that, a system is registered in the `SystemCatalog` with
`E_SYSTEM`, in its header, after the class:

```cpp
// Movement.hpp
class MovementSystem : public ECS::ISystem {
    public:
        void update(ECS::Registry& world, double dt) override;   // may be defined in Movement.cpp
        const std::string name() const override { return "MovementSystem"; }
};
E_SYSTEM(MovementSystem)   // registered before main, under "MovementSystem"
```

```cpp
auto& catalog = ECS::SystemCatalog::instance();
catalog.createInto("MovementSystem", systems, world);   // creates it, adds it last, calls init
catalog.names();                                       // every registered name, sorted

systems.getSystem("MovementSystem");                   // first system with that name(), or nullptr
systems.removeSystem(world, "MovementSystem");         // calls its shutdown, removes the first one
```

- **The name given to `E_SYSTEM` must be exactly what `name()` returns.** It
  is the macro argument as written, each run of spaces reduced to one:
  `E_SYSTEM(game::MovementSystem)` gives `"game::MovementSystem"`, but
  `E_SYSTEM(::game::MovementSystem)` gives `"::game::MovementSystem"` and
  `E_SYSTEM(game :: MovementSystem)` gives `"game :: MovementSystem"`. Write it
  with no leading `::` and no spaces. Scene files store the name, so
  `catalog.create()` checks that `name()` matches: when it does not, the
  system is refused (`nullptr`, nothing added) and the mismatch is listed in
  `catalog.errors()`. The check runs in every build.
- Qualify the name to avoid collisions between modules.
- A template system can be registered: `E_SYSTEM(game::Spawner<Enemy, 3>)`
  gives `"game::Spawner<Enemy, 3>"`, with the spaces after the commas as
  written.
- A system created by name needs a default constructor.
- A name registered twice is not an exception (it would happen before `main`):
  the first system is kept, and the error is listed in
  `SystemCatalog::instance().errors()` and printed on `std::cerr`.
- `catalog.add<T>("Name")` or `catalog.add("Name", factory)` register a system
  by hand.
- `createInto` called twice with the same name adds **two instances** of the
  system: a `SystemManager` may hold several systems with the same name.
  `getSystem(name)` and `removeSystem(world, name)` act on the first one.

**Put `E_SYSTEM` in the header, and make sure the header is compiled.** Nothing
in the code references a system that is only chosen by name, so the linker
may drop it:

| The game's systems are... | Registered? |
| --- | --- |
| compiled into the executable | yes: every object file of the executable is kept |
| in a shared library (plugins) | yes: the library is loaded whole |
| in a static library | only if their headers are included from a `.cpp` of the executable (for example one `Systems.hpp` that includes them all), or if the library is linked with `WHOLE_ARCHIVE` |

The last case goes away once the game's code is a shared library.

### Changing the world during a loop

Spawning, killing, adding or removing components while a view is being iterated
would move the arrays under the loop. Inside `runDeferred`, these changes are
queued and applied in order when it ends:

```cpp
world.runDeferred([&] {
    for (auto [entity, health] : world.view<Health>()) {
        if (health.current <= 0) {
            world.killEntity(entity);           // queued
            world.spawnEntity("loot");          // queued
        }
    }
});                                             // applied here
```

**Systems don't need to do it**: `SystemManager::update` runs each system inside
its own `runDeferred`, and applies its changes before the next system starts.
A `runDeferred` called inside another one joins it, so a helper can always use
one safely.

### Resources

A resource is a single value shared by the whole world, not attached to an
entity (time, input, settings):

```cpp
struct Time { double dt = 0.0; };

world.setResource<Time>(Time{0.016});
double dt = world.getResource<Time>().dt;
bool hasTime = world.hasResource<Time>();
```

## Full example

```cpp
#include <iostream>
#include <string>

#include "ECS/ECS.hpp"

struct Position { float x = 0; float y = 0; };
struct Velocity { float dx = 0; float dy = 0; };
struct Health { int current = 100; };
struct Time { double dt = 0.0; };

class MovementSystem : public ECS::ISystem {
    public:
        void update(ECS::Registry& world, double) override {
            const double dt = world.getResource<Time>().dt;
            for (auto [entity, position, velocity] : world.view<Position, Velocity>()) {
                position.x += velocity.dx * dt;
                position.y += velocity.dy * dt;
            }
        }
        const std::string name() const override { return "MovementSystem"; }
};

class DeathSystem : public ECS::ISystem {
    public:
        void update(ECS::Registry& world, double) override {
            for (auto [entity, health] : world.view<Health>()) {
                if (health.current <= 0) {
                    world.killEntity(entity);   // applied after this system
                }
            }
        }
        const std::string name() const override { return "DeathSystem"; }
};

int main() {
    ECS::Registry world;
    world.setResource<Time>(Time{0.5});

    const ECS::Entity player = world.spawnEntity("player");
    world.addComponent(player, Position{0, 0});
    world.addComponent(player, Velocity{2, 0});
    world.emplaceComponent<Health>(player, 100);

    const ECS::Entity enemy = world.spawnEntity("enemy");
    world.emplaceComponent<Health>(enemy, 0);

    ECS::SystemManager systems;
    systems.addSystem<MovementSystem>(world);
    systems.addSystem<DeathSystem>(world);
    systems.update(world);

    std::cout << "player x = " << world.getComponent<Position>(player).x   // 1
              << ", enemy alive = " << world.isAlive(enemy)                // 0
              << ", entities = " << world.entityCount() << '\n';           // 1
}
```

## Rules to remember

- **Keep entities, not component references.** A reference returned by
  `getComponent` or `addComponent` is valid until the next add or remove of that
  component type, or the end of the current `runDeferred`. To remember
  something across frames, keep the `Entity` and fetch the component again
  (check `isAlive` first).
- **Inside a system, changes apply after the system.** An entity killed in a
  system is still alive until that system returns; one spawned there does not
  exist yet (but can already receive components).
- **Components must be movable.** They are moved inside their array; a type
  that cannot be moved is rejected at compile time with a clear message.
- **A `Registry` cannot be copied or moved.** Views, queued changes and tools
  point into it. Keep it as a member of its scene, or behind a
  `std::unique_ptr` to store several.
- **Handles belong to their registry.** An entity from another registry is
  rejected (`std::invalid_argument`), even if its index matches.
- **Do not add or remove systems from a system's `update`.** `SystemManager`
  is iterating over its systems: changing the list then is undefined behaviour.
  Add them before the loop (scene creation, `onEnter`) or between two frames.
- **`SystemCatalog` is not thread-safe.** `E_SYSTEM` registrations run before
  `main`, on one thread; do not register systems from several threads.

## Going further

- **Tools (editor, save files, debug console)** use the reflection layer and
  `world.inspect()` to read and edit components without knowing their types:
  see [Reflection/README.md](Reflection/README.md).
- **How it works inside**: each class is documented in its header.

| Folder | Content |
| --- | --- |
| [Core/](Core/) | [Registry.hpp](Core/Registry.hpp) (the world) and [CommandBuffer.hpp](Core/CommandBuffer.hpp) (deferred changes) |
| [Core/Entity/](Core/Entity/) | `Entity`, `EntityManager` (entity lifecycle), `EntityList`, `Name` |
| [Core/Component/](Core/Component/) | `ComponentStorage` (one dense array per type), `ComponentStore` (all of them), `SignatureTable` (which entity has which component) |
| [Core/Query/](Core/Query/) | `View` and the `Zipper` that walks several arrays at once |
| [Core/Resource/](Core/Resource/) | `ResourceStore` |
| [Core/System/](Core/System/) | `ISystem`, `SystemManager` (systems found and removed by type or by name), `SystemCatalog` and `E_SYSTEM` (systems created by name) |

The ECS is header-only: [CMakeLists.txt](CMakeLists.txt) defines an `INTERFACE`
target that only exposes the include directory.
