# Reflection

Runtime reflection for the ECS. Types described with `E_REFLECT` can be read
and modified by code that does not know them at compile time.

Everything lives in the `ECS::reflect` namespace. The ECS core does not depend
on this directory.

## Contents

1. [Why reflection?](#why-reflection)
2. [Key concepts](#key-concepts)
3. [Usage](#usage)
   1. [Reflect a type](#1-reflect-a-type)
   2. [Give it to an entity](#2-give-it-to-an-entity)
   3. [Get it back without knowing its type](#3-get-it-back-without-knowing-its-type)
   4. [Manipulate it](#4-manipulate-it)
4. [How it works](#how-it-works)
5. [Files](#files)
6. [Rules](#rules)

## Why reflection?

In C++, a type exists only at compile time. Code that does not include
`Health.hpp` cannot read `health.current`. Reflection keeps a description of
each type at runtime: its name, its fields and how to reach them. With that
description, code can use a type it only knows by name.

| Code that knows its types | Code that only has a type *name* |
| --- | --- |
| gameplay systems, C++ scripts | editor, interpreted scripting binding, scene loading, serialization, debug console, a module using a component whose header it does not have |
| typed `Registry` API: `world.spawnEntity()`, `world.getComponent<Health>(e)`, `world.view<...>()` | reflection: `world.inspect()`, `Ref` |

Both sides use the same storage. A component added through reflection can be
read with `getComponent<T>`, and the other way round.

## Key concepts

| Concept | What it is |
| --- | --- |
| **`E_REFLECT`** | Macro that describes a type and its fields. It is written next to the type and does not modify it. |
| **`TypeInfo`** | Runtime description of a type: `name`, `id`, `size`, `kind`, `fields` and the operations the kind needs (construct, destroy, copy, container access). You never build one yourself. |
| **`TypeKind`** | How a type can be inspected: `Struct` (reflected), `Scalar` (`bool`, integers, `float`, `double`, `std::string`), `Sequence` (`vector`, `array`), `Map` (`map`, `unordered_map`), `Optional`, `Enum` (through its integer value), `Opaque` (known size, contents not inspectable). |
| **`TypeId`** | Hash of the reflected name. It is stable across runs and builds, so it can be persisted, unlike `componentId<T>()`. |
| **`TypeRegistry`** | Process-wide catalogue of every `TypeInfo`, found by type, name or `TypeId`. |
| **`Ref`** | Non-owning view on a value: a `TypeInfo` plus an address. You navigate through it by field name, index or key, and read the value back with `as<T>()`. |
| **`RegistryInspector`** | Returned by `world.inspect()`. Gives access to an entity's components from a `TypeInfo` (`list`, `get`, `has`, `add`, `remove`), lists component types (`types`, `findType`), and lists the entities (`entities`, an always up-to-date `EntityList`) with a label for each (`displayName`: its `ECS::Name`, or `"Entity <index>"`). |
| **`ContainerTraits`** | Customisation point. Specialise it to make your own container inspectable. |
| **PrettyPrint** | Optional. Prints any `Ref` in a readable form, for logs and debugging. |

## Usage

The examples below share these types:

```cpp
#include "ECS/Reflection/Reflection.hpp"
#include "ECS/Reflection/PrettyPrint.hpp"   // optional, for std::cout << ref

namespace reflect = ECS::reflect;

struct Health { int current = 100; int max = 100; };
struct Item { std::string id; int count = 1; };
struct Inventory { std::vector<Item> items; std::optional<Item> equipped; };
```

### 1. Reflect a type

```cpp
E_REFLECT(Health)
    E_FIELD(current, int)
    E_FIELD(max, int)
E_END

E_REFLECT(Item)
    E_FIELD(id, std::string)
    E_FIELD(count, int)
E_END

E_REFLECT(Inventory)
    E_FIELD(items, std::vector<Item>)
    E_FIELD(equipped, std::optional<Item>)
E_END
```

- `E_REFLECT` goes at global scope, after the type. It does not modify the
  type, so it also works for third-party types.
- Fields must be accessible: public, or the type befriends
  `ECS::reflect::Descriptor<T>`.
- `E_FIELD` checks at compile time that the declared type matches the real
  one.
- A field can be a scalar, another reflected type, a `std::vector`,
  `std::array`, `std::map`, `std::unordered_map` or `std::optional` (nested as
  deep as needed), or an enum. Anything else is `Opaque`. Support for other
  containers is added by specialising `ContainerTraits`.

In this ECS a component is any data struct attached to an entity, so every
`E_REFLECT` type is **also registered as a component, before `main`**, with no
registration call needed.

### 2. Give it to an entity

When the type is known, use the typed API:

```cpp
ECS::Entity player = world.spawnEntity("player");
world.addComponent(player, Health{75, 100});
```

When only the name is known (editor menu, scene file, script):

```cpp
auto inspector = world.inspect();

const reflect::TypeInfo* type = reflect::RegistryInspector::findType("Inventory");
reflect::Ref inventory = inspector.add(player, *type);   // default-constructed, pool created if needed
```

`RegistryInspector::types()` lists every component type, for example to fill
an "Add component" menu.

### 3. Get it back without knowing its type

**All of an entity's components**, for example to show them in an editor
panel:

```cpp
for (reflect::Ref component : inspector.list(player)) {
    std::cout << component.type().name << "\n";   // "Health", "Inventory"
}
```

**One component, by type name**:

```cpp
const reflect::TypeInfo* health = reflect::RegistryInspector::findType("Health");   // nullptr if unknown

if (health && inspector.has(player, *health)) {
    reflect::Ref ref = inspector.get(player, *health);   // see step 4 to use it
}
```

`get` returns an invalid `Ref` when the entity does not have the component, so
the `has` check is optional.

**Removing a component** works the same way:

```cpp
inspector.remove(player, *health);   // same as world.removeComponent<Health>(player)
```

`world.inspect()` returns a lightweight handle that references `world`. Create
it where you need it. It must not outlive the `Registry`.

### 4. Manipulate it

**By path**, when the field names are known:

```cpp
reflect::Ref root = inspector.get(player, *reflect::RegistryInspector::findType("Inventory"));
// or, on any value: reflect::Ref root = reflect::Ref::make(myInventory);

root.path("items").push().path("id").as<std::string>() = "sword";
root.path("items[0].count").as<int>() += 1;
root.path("equipped").emplace();
root.path("equipped.id").as<std::string>() = "shield";   // goes through the optional
root.path("items").eraseAt(0);

std::cout << root << "\n";   // PrettyPrint.hpp
```

**Generically**, when nothing is known (an editor panel, a serializer):

```cpp
void walk(reflect::Ref value) {
    if (value.is(reflect::TypeKind::Struct)) {
        for (const reflect::FieldInfo& field : value.type().fields) {
            std::cout << field.name << ": " << field.type->name << "\n";
            walk(value.field(field));
        }
    } else if (int* number = value.tryAs<int>()) {
        *number = 0;
    }   // Sequence, Map, Optional: see the table below
}
```

- `as<T>()` throws when the value is not exactly a `T`. `tryAs<T>()` returns
  `nullptr` instead.
- A missing field, out-of-range index, missing key or empty optional gives an
  invalid `Ref` (`if (ref)`). Lookups can therefore be chained safely.
- Path syntax: `a.b`, `list[2]`, `map[key]` (string, integer or enum keys).

| Kind | Operations on `Ref` |
| --- | --- |
| any | `valid`, `is(kind)`, `type`, `data`, `as<T>`, `tryAs<T>` |
| struct | `field(name)`, `field(FieldInfo)`, `path(...)` |
| sequence (`vector`, `array`) | `size`, `at`, `resizable`, `resize`, `push`, `eraseAt` |
| map | `size`, `find(key)`, `insert(key)`, `eraseKey(key)`, `forEachEntry` |
| optional | `hasValue`, `value`, `emplace`, `reset` |
| enum | `enumValue`, `setEnumValue` |

For printing, `PrettyPrint.hpp` also provides `prettyPrint(os, ref, options)`
and `toPrettyString(ref, options)`. `PrettyPrintOptions` controls `indent`,
`showTypes`, `maxElements` and `compactLeafSequences`.

## How it works

```
 compile time                     before main (static init)               runtime
┌──────────────────┐         ┌───────────────────────────────┐
│ E_REFLECT(Health)│         │ TypeRegistry                  │
│   E_FIELD(...)   │────────▶│  TypeInfo "Health"            │◀─── TypeRegistry::find(name/id)
│ E_END            │ register│   fields → TypeInfo "int32"…  │     Ref::make(value)
│                  │  Type   │   (field types registered     │
│ = Descriptor<T>  │         │    recursively)               │
└──────────────────┘         └───────────────┬───────────────┘
                                             │ if the type can live in a pool
                                             ▼
                             ┌───────────────────────────────┐
                             │ ComponentCatalog (detail/)    │
                             │  TypeInfo* + ComponentId      │
                             │  + ComponentOps (typed code   │
                             │    compiled for this type)    │
                             └───────────────┬───────────────┘
                                             │
                                             ▼
          world.inspect() ─▶ RegistryInspector ─▶ Registry::*ById ─▶ ComponentPool<Health>
                                    │
                                    ▼
                             Ref { TypeInfo*, void* } ─▶ field / path / at / find ─▶ as<T>()
```

1. `E_REFLECT` specialises `Descriptor<T>`, a compile-time list of fields.
   It also defines a static variable whose initialisation calls
   `detail::registerReflectedType<T>()` before `main`.
2. That call builds the type's `TypeInfo` in the `TypeRegistry`, registering
   the field types recursively. Each field gets a captureless accessor, and
   each container kind gets function pointers generated from
   `ContainerTraits`.
3. If the type can be stored in a pool (`ComponentStorable`), the
   `ComponentCatalog` also records it, with its `ComponentId` and the typed
   operations (`ComponentOps`) the `Registry` needs.
4. At runtime, `RegistryInspector` turns a `TypeInfo` into a `ComponentId`
   and its operations, and calls the `Registry`'s `*ById` functions. The
   address it gets back is wrapped in a `Ref`, which uses the `TypeInfo` to
   reach fields and elements.

The detailed documentation of every class and function is in the headers, as
docstrings. This README only gives the overview.

## Files

| File | Purpose |
| --- | --- |
| [Reflection.hpp](Reflection.hpp) | Entry point. Includes everything below except `PrettyPrint.hpp`. |
| [Descriptor.hpp](Descriptor.hpp) | `E_REFLECT` / `E_FIELD` / `E_END` macros, `Descriptor<T>` and compile-time visitors (`forEachField`). |
| [TypeInfo.hpp](TypeInfo.hpp) | Runtime description of a type: kind, fields, container operations, lifetime operations, stable `TypeId`. |
| [TypeRegistry.hpp](TypeRegistry.hpp) | Process-wide registry of `TypeInfo`, by type, name or `TypeId`. |
| [ContainerTraits.hpp](ContainerTraits.hpp) | Customisation point that makes a container inspectable. |
| [Ref.hpp](Ref.hpp) | Type-erased view on a value. |
| [RegistryInspector.hpp](RegistryInspector.hpp) | Components of a `Registry` by `TypeInfo`. Defines `Registry::inspect()` and `registerComponentType<T>()`. |
| [CoreTypes.hpp](CoreTypes.hpp) | Reflection of the core's own components (`ECS::Name`). Included by `Reflection.hpp`. |
| [PrettyPrint.hpp](PrettyPrint.hpp) | Optional. Readable printing of any reflected value, for logs and debugging. |
| [detail/](detail/) | Internals (component catalog, automatic registration). Not meant to be included directly. |

## Rules

- **`Ref`s to components are short-lived.** Any structural change of that
  component type invalidates them, as it does a reference from
  `Registry::getComponent<T>`. Fetch them again rather than keeping them
  across frames. A scripting binding should keep "entity + type", not a `Ref`.
- **Adding a component by type needs a default constructor.** Without one,
  `inspector.add` returns an invalid `Ref`. A type that cannot live in a pool
  (for example one with a `const` member, see `ComponentStorable`) is
  reflected but is not a component. Const members can be read but not written
  (`FieldInfo::readOnly`).
- **Under `runDeferred`**, `inspector.add` returns a `Ref` to the value that
  will be applied at flush, like `Registry::addComponent<T>`.
- **Automatic registration runs from static initialisation.** A translation
  unit dropped by the linker (an unreferenced object file of a static
  library) never registers its types. Link such a library with
  `WHOLE_ARCHIVE`, or call `reflect::registerComponentType<T>()`.
- **Registration errors cannot throw before `main`** (for example, two types
  reflected under the same name). Check `RegistryInspector::errors()` at
  startup.
- **Names are identities.** A type's `TypeId` is the hash of its reflected
  name. Give reflected types unique names, and qualify them when needed
  (`E_REFLECT(game::Position)`). Enum and Opaque type names come from the
  compiler and must not be persisted.
- **Registration is not thread-safe.** It happens at startup.
