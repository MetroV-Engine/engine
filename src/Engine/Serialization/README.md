# Serialization

Saves a world, or any reflected value, as text, and reads it back
(part of `engine_core`). It is what scene files, the editor's "Save" and,
later, the player's saves are made of.

The whole public API is [Serialization.hpp](Serialization.hpp), in the
`serial` namespace. It exposes no JSON type: the format and the library
behind it (nlohmann-json) are private, so they can change without touching a
caller.

| Function | Does |
| --- | --- |
| `serialize(world)` | the world as text |
| `load(world, text)` | creates the entities of the text, in addition to the existing ones |
| `saveFile(world, path)` / `loadFile(world, path)` | the same, through a file (written to a temporary file then renamed) |
| `serialize(ref)` / `load(ref, text)` | one reflected value (a component, a field) |

Each takes an optional `Report*`: `errors` (the call failed) and `warnings`
(something was skipped or defaulted, the call went on). A `Report` is only ever
appended to, so one can collect several calls: a call succeeds when it adds no
error, whatever the `Report` already held.

```cpp
serial::Report report;
if (!serial::loadFile(world, "levels/level1.json", &report))
    for (const std::string& error : report.errors) std::cerr << error << "\n";
for (const std::string& warning : report.warnings) std::cerr << warning << "\n";
```

## Entity ids

An `ECS::Entity` handle only means something inside its Registry: after a
reload its index, generation and WorldId are all different. Entities are
therefore identified in the text by an `Uuid`, a component of the entity
(reflected like `ECS::Name`). A reference between entities (a field of type
`ECS::Entity`) is written as the Uuid of its target.

- An entity without a Uuid gets a temporary one **in the text only**; the
  world is not modified, and saving twice writes two different ids.
- Today only the editor's scenes are saved: everything the editor creates is
  persistent, and what runs in a simulation or a game is never written. Saving
  a game in progress (and leaving out transient entities such as bullets) is a
  separate subject, not handled yet.
- To make the ids stable (an editor saving a scene), pass
  `SaveOptions{.assignMissingUuids = true}`: the entities without a Uuid get
  one, once, and every later save writes the same ids. An entity whose Uuid is
  used by another entity is left as it is, with a warning.
- On load, every entity gets its Uuid, and references are rebuilt in two
  passes (all entities are created first), so they can point forward.

## Format (version 1)

```json
{
  "format": "metrov.world",
  "version": 1,
  "entities": [
    {
      "id": "3f2a9c1e-5b7d-4e08-9a41-c2d6e8f01b37",
      "components": {
        "ECS::Name": { "value": "player" },
        "Health": { "current": 75, "max": 100 },
        "Follow": { "target": "9d41b0c7-2e6f-4a13-8c55-1f7a3b9e4d20" }
      }
    }
  ]
}
```

- Components are keyed by their reflected name; fields keep their declaration order.
- Scalars are JSON values. Enums are their integer value. `std::vector` and
  `std::array` are arrays, `std::optional` is the value or `null`, maps are
  objects (keys: string, integer or enum, written sorted).
- A `char` is written as a code from 0 to 255, whatever the sign of `char` on
  the platform.
- A non-finite number is written as `null`, with a warning.
- A `const` member (a read-only field) is not written, and a text that has
  one gets a warning: it cannot be restored.
- Numbers are written and read with a dot whatever the C locale.
- `Opaque` types (not reflected) and unsupported scalars (`long double`,
  `wchar_t`...) are skipped, with a warning.

## Reading is tolerant

An unknown component or field, or a value of the wrong type, is a warning and
the default value stays. So does a number that does not fit its field: an
integer out of range, a value too large for an enum's type or for a `float`
(it is never truncated or turned into infinity). These are errors, and nothing is created: text that is
not JSON, a wrong `format`, a `version` newer than this build, an entity
without a valid `id`, an id that appears twice or is already in the world.

## Loading into an existing value

For `load(Ref, text)`: a struct field absent from the text keeps its current
value, but a container (`std::vector`, `std::map`...) takes exactly the
content of the text: it is resized, or emptied then filled.

## Files

`saveFile` writes a temporary file of its own name next to the target, flushes
it to the disk (`fsync`), renames it over the target, then syncs the folder
(where the system allows): a crash leaves the previous file or the new one,
never half of it. On failure the target is untouched.

## Limits

- `load` must not be called inside `Registry::runDeferred`. The Registry does
  not expose whether it is recording, so this is not detected.
- Loading adds to the world; to replace it, load into a new `Registry`.
- No `Uuid` -> `Entity` index yet: ids are matched by scanning the world once per call.
