# Reflection

Runtime reflection for the ECS: types described with `E_REFLECT` can be read
and modified by code that does not know them at compile time.

Everything lives in the `ECS::reflect` namespace. The ECS core does not depend
on this directory.

## Entry points

| I want to… | I use… |
| --- | --- |
| make a type reflectable (and usable as a component by name) | `E_REFLECT` / `E_FIELD` / `E_END`, after `#include "ECS/Reflection/Reflection.hpp"` |
| read or modify a value without knowing its type | `reflect::Ref`: `Ref::make(value)`, `path("a.b[0]")`, `as<T>()` |
| read or modify the components of an entity without knowing their types | `world.inspect()`: `list`, `get`, `has`, `add`, `remove` |
| list the component types, or find one by name | `reflect::RegistryInspector::types()`, `findType(name)` |
| print any reflected value | `std::cout << ref` (optional `ECS/Reflection/PrettyPrint.hpp`) |

**Which API for which code?** Code that knows its types (gameplay systems, C++
scripts) uses the typed `Registry` API: `world.spawnEntity()`,
`world.getComponent<Health>(e)`, `world.view<...>()`. Reflection is for code
that only has a type *name* or `TypeInfo`: the editor, an interpreted scripting
binding, scene loading, serialization, a debug console, or a module using a
component whose header it does not have.

## Reflecting a type

```cpp
#include "ECS/Reflection/Reflection.hpp"

struct Item { std::string id; int count = 1; };
struct Inventory { std::vector<Item> items; std::optional<Item> equipped; };

E_REFLECT(Item)
    E_FIELD(id, std::string)
    E_FIELD(count, int)
E_END

E_REFLECT(Inventory)
    E_FIELD(items, std::vector<Item>)
    E_FIELD(equipped, std::optional<Item>)
E_END
```

`E_REFLECT` is written at global scope, after the type. It does not modify the
type, so it also works for third-party types. Fields must be accessible (public,
or the type befriends `ECS::reflect::Descriptor<T>`).

Field types can be scalars (`bool`, integers, `float`, `double`,
`std::string`), other reflected types, `std::vector`, `std::array`, `std::map`,
`std::unordered_map`, `std::optional` (nested as deep as needed), and enums
(handled through their integer value). Anything else is `Opaque`: known size,
not inspectable. Other containers can be supported by specialising
`ContainerTraits`.

In this ECS a component is any data struct attached to an entity, so every
`E_REFLECT` type is also registered as a component, before `main`, without any
registration call.

## Values: `Ref`

```cpp
namespace reflect = ECS::reflect;

Inventory inventory;
reflect::Ref root = reflect::Ref::make(inventory);

root.path("items").push().path("id").as<std::string>() = "sword";
root.path("items[0].count").as<int>() += 1;
root.path("equipped").emplace();
root.path("items").eraseAt(0);

std::cout << root << "\n";   // PrettyPrint.hpp
```

`as<T>()` throws when the value is not exactly a `T`; `tryAs<T>()` returns
`nullptr` instead. A missing field, index or key gives an invalid `Ref`.

| Value kind | Operations |
| --- | --- |
| struct | `field(name)`, `path(...)` |
| sequence (`vector`, `array`) | `size`, `at`, `resizable`, `resize`, `push`, `eraseAt` |
| map | `size`, `find(key)`, `insert(key)`, `eraseKey(key)`, `forEachEntry` |
| optional | `hasValue`, `value`, `emplace`, `reset` |
| enum | `enumValue`, `setEnumValue` |

## Components: `world.inspect()`

```cpp
auto inspector = world.inspect();

for (reflect::Ref component : inspector.list(player)) {   // every component of an entity
    std::cout << component << "\n";
}

const reflect::TypeInfo* health = reflect::RegistryInspector::findType("Health");
inspector.get(player, *health).path("current").as<int>() -= 10;
inspector.add(player, *reflect::RegistryInspector::findType("Inventory"));   // creates the pool if needed
inspector.remove(player, *health);

for (const reflect::TypeInfo* type : reflect::RegistryInspector::types()) { /* "Add component" menu */ }
```

`world.inspect()` returns a lightweight handle referencing `world`: create it
where needed, it must not outlive the `Registry`. It is the only way to get a
`RegistryInspector`. Both APIs share one implementation.

## Files

| File | Purpose |
| --- | --- |
| [Reflection.hpp](Reflection.hpp) | Entry point: includes everything below except `PrettyPrint.hpp`. |
| [Descriptor.hpp](Descriptor.hpp) | `E_REFLECT` / `E_FIELD` / `E_END` macros, `Descriptor<T>` and compile-time visitors (`forEachField`). |
| [TypeInfo.hpp](TypeInfo.hpp) | Runtime description of a type: kind, fields, container operations, lifetime operations, stable `TypeId`. |
| [TypeRegistry.hpp](TypeRegistry.hpp) | Process-wide registry of `TypeInfo`, by type, name or `TypeId`. |
| [ContainerTraits.hpp](ContainerTraits.hpp) | Customisation point making a container inspectable. |
| [Ref.hpp](Ref.hpp) | Type-erased view on a value. |
| [RegistryInspector.hpp](RegistryInspector.hpp) | Components of a `Registry` by `TypeInfo`; defines `Registry::inspect()`. |
| [PrettyPrint.hpp](PrettyPrint.hpp) | Optional: readable printing of any reflected value, for logs and debugging. |
| [detail/](detail/) | Internals (component catalog, automatic registration). Not meant to be included directly. |

## Rules

- **`Ref`s to components are short-lived.** They are invalidated by any
  structural change of that component type, like a reference from
  `Registry::getComponent<T>`: fetch them again rather than keeping them across
  frames (a scripting binding should keep "entity + type", not a `Ref`).
- **Adding a component by type needs a default constructor.** A type that
  cannot live in a pool (e.g. a `const` member, see `ComponentStorable`) is
  reflected but is not a component. Const members are reachable read-only
  (`FieldInfo::readOnly`).
- **Automatic registration runs from static initialisation.** A translation
  unit dropped by the linker (an unreferenced object file of a static library)
  never registers its types: link such a library with `WHOLE_ARCHIVE`, or call
  `reflect::registerComponentType<T>()`.
- **Registration errors cannot throw before `main`** (e.g. two types reflected
  under the same name): check `RegistryInspector::errors()` at startup.
- **Names are identities.** A type's `TypeId` is the hash of its reflected name,
  stable across runs: give reflected types unique names (qualify them, e.g.
  `E_REFLECT(game::Position)`, when needed). Enum and Opaque type names come
  from the compiler and must not be persisted.
- **Registration is not thread-safe**: it happens at startup.
