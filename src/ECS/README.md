# ECS

This directory contains the header-only Entity Component System used by the
engine.

The ECS headers contain detailed Doxygen documentation for classes, methods,
parameters, ownership rules, and important implementation details. Reading the
code is therefore also a useful way to understand the exact behavior of the
system.

## ECS Overview

The ECS separates an entity's identity from the data attached to it:

```text
Entity
  |
  +-- Position component
  +-- Velocity component
  +-- Health component
```

An entity is only an identity. Components contain data, and systems operate on
entities that contain the components they need.

## Main Files

| File | Purpose |
| --- | --- |
| [Entity.hpp](Entity.hpp) | Lightweight entity handle and identity access. |
| [EntityManager.hpp](EntityManager.hpp) | Entity creation, destruction, validation, and ID reuse. |
| [ComponentId.hpp](ComponentId.hpp) | Lazy runtime ID assignment for component types. |
| [SparseSet.hpp](SparseSet.hpp) | Dense storage and fast entity-to-component lookup. |
| [ComponentPool.hpp](ComponentPool.hpp) | Type-erased access to different component pools. |
| [Registry.hpp](Registry.hpp) | Main ECS API for entities, components, and systems. |
| [Zipper.hpp](Zipper.hpp) | Iteration over entities shared by several component pools. |

## Entity

See [Entity.hpp](Entity.hpp).

`Entity` is a lightweight handle containing only a numeric identity. It can be
copied cheaply and passed to component operations. Display names are not stored
in the handle; they are owned by `Registry`.

```cpp
ECS::Entity player = world.spawnEntity("Player");
std::size_t id = player.value();
```

The entity handle does not create or destroy entities. That responsibility
belongs to `EntityManager` and `Registry`.

```text
Entity handle -> numeric ID
               -> component pool lookup
```

The current handle uses an ID only. Generation-based stale-handle protection is
not implemented yet, so a handle must not be used after its entity is destroyed.

## EntityManager

See [EntityManager.hpp](EntityManager.hpp).

`EntityManager` owns the lifecycle of entity IDs. It tracks live IDs and keeps a
free list for IDs that can be reused.

```cpp
ECS::Entity first = manager.create();
manager.destroy(first);
ECS::Entity recycled = manager.create();
```

The usual flow is:

```text
create -> live entity
destroy -> free ID
create -> reuse free ID when available
```

Destroying an entity in `EntityManager` does not remove its components. The
`Registry` performs both operations together.

## Component IDs

See [ComponentId.hpp](ComponentId.hpp).

Each component type receives a numeric ID the first time `componentId<T>()` is
used. No manual registration line is required for each component.

```cpp
ECS::ComponentId positionId = ECS::componentId<Position>();
ECS::ComponentId velocityId = ECS::componentId<Velocity>();
```

The same type always returns the same ID during the process:

```text
Position -> ComponentId 0
Velocity -> ComponentId 1
Position -> ComponentId 0
```

These IDs are runtime implementation details. They must not be stored in scene
files, save files, or network data.

## SparseSet

See [SparseSet.hpp](SparseSet.hpp).

`SparseSet<Component>` stores components densely while keeping a lookup from an
entity ID to the component's packed position.

```text
Entity ID       0     1     2
Sparse lookup   -     0     1

Packed data   [Position of entity 1][Position of entity 2]
```

Typical operations are:

```cpp
ECS::SparseSet<Position> positions;
positions.emplaceAt(entityId, 10, 20);

if (positions.has(entityId)) {
    Position& position = positions.get(entityId);
}
```

The packed storage improves iteration performance and cache locality. Removing
a component uses swap-and-pop, so packed order is not stable after an erase.

## ComponentPool

See [ComponentPool.hpp](ComponentPool.hpp).

The registry stores pools containing different component types. Since one
container cannot directly store `SparseSet<Position>` and `SparseSet<Velocity>`
as the same concrete type, `IComponentPool` provides a small type-erased
interface.

```text
Registry
  |
  +-- ComponentPool<Position>
  +-- ComponentPool<Velocity>
  +-- ComponentPool<Health>
```

Typed access remains available through the registry, while type erasure is used
for operations shared by every pool, such as removing a destroyed entity.

## Registry

See [Registry.hpp](Registry.hpp).

`Registry` is the main entry point for the ECS. It coordinates entity lifecycle,
component pools, component access, and systems.

### Creating and destroying entities

```cpp
ECS::Registry world;

ECS::Entity player = world.spawnEntity();
world.killEntity(player);
```

An entity may receive an optional display name when it is created:

```cpp
ECS::Entity player = world.spawnEntity("Player");
world.setEntityName(player, "MainPlayer");
std::string name = world.getEntityName(player);
```

The name is registry metadata. It does not affect the entity ID or component
storage.

When `killEntity` is called, the registry:

1. destroys the entity through `EntityManager`;
2. removes the entity from every registered component pool;
3. removes its optional name;
4. returns its ID to the free list.

### Adding components

Use `emplaceComponent` when the registry should construct the component from
constructor arguments:

```cpp
world.emplaceComponent<Position>(player, 100, 200);
```

Use `addComponent` when a component object already exists:

```cpp
Position randomPosition{randomX(), randomY()};
world.addComponent(player, randomPosition);
```

Both operations add or replace the component for the selected entity.

### Accessing and removing components

```cpp
if (world.hasComponent<Health>(player)) {
    Health& health = world.getComponent<Health>(player);
}

world.removeComponent<Health>(player);
```

`getComponent` throws `std::out_of_range` when the component pool or component
does not exist for the entity. `getIf` is available when a nullable lookup is
preferred.

## Systems

Systems are registered on [Registry.hpp](Registry.hpp) with the component pools
they require. They are executed in registration order.

```cpp
world.addSystem<Position, Velocity>(
    [](ECS::Registry&, ECS::SparseSet<Position>& positions,
       ECS::SparseSet<Velocity>& velocities) {
        for (auto [position, velocity, entityId] :
             ECS::zipper(positions, velocities)) {
            position.x += velocity.x;
            position.y += velocity.y;
        }
    });

world.runSystems();
```

The registry supplies the requested typed pools to the system:

```text
Registry
  -> Position pool
  -> Velocity pool
  -> system processes matching entities
```

## Zipper

See [Zipper.hpp](Zipper.hpp).

`zipper` iterates the intersection of several component pools. It uses the first
pool as the driving set and checks whether each entity also exists in the other
pools.

```cpp
for (auto [position, velocity, entityId] :
     ECS::zipper(positions, velocities)) {
    position.x += velocity.x;
}
```

Only entities with both components are visited:

```text
Position: entity 0, entity 1, entity 2
Velocity: entity 1, entity 2
Result:   entity 1, entity 2
```

The first pool should generally be a small or selective pool when that choice is
available, because it determines the number of membership checks.

## Header-Only Design

The ECS is implemented in headers so that its templates and generic operations
are available wherever the ECS is included. There are no ECS `.cpp` files.

The CMake target in [CMakeLists.txt](CMakeLists.txt) is an `INTERFACE` target and
only exposes the include directory.

```text
ECS headers
    |
    +-- include directly from user code
    +-- no ECS object file
    +-- template code compiled where used
```

## Basic Usage

The minimal ECS workflow is:

```cpp
ECS::Registry world;
ECS::Entity entity = world.spawnEntity();

world.emplaceComponent<Position>(entity, 0, 0);
world.emplaceComponent<Velocity>(entity, 1, 0);

world.addSystem<Position, Velocity>(updatePositions);
world.runSystems();

world.killEntity(entity);
```

The important relationship is:

```text
Registry
  -> EntityManager manages entity IDs
  -> ComponentPool stores component data
  -> SparseSet provides dense storage
  -> Zipper helps systems iterate matching entities
```
