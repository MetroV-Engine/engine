# Engine

The runtime library of a game (`engine_core`): the main loop and the scenes.
No editor, no ImGui. The editor (`engine_editor`) and the executables link it.

| File | Content |
| --- | --- |
| `Engine.hpp/.cpp` | `Engine`: `init()`, `step(dt)` (one frame, for a host with its own loop such as the editor), `run()` (calls `step` until `stop()` or no scene is left), `setTargetFrameRate(fps)` / `setMaxFrameTime(s)` (the pace of `run()`), `shutdown()`, `scenes()`, `services()` / `addService<T>()` |
| `Scenes/Scene.hpp` | `Scene`: a world (its own `ECS::Registry` and `SystemManager`) with `onEnter` / `onExit` / `onPause` / `onResume` hooks |
| `Scenes/DataScene.hpp/.cpp` | `DataScene`: a scene built from a scene file (see below). `addDataScene(registry, id, file, setup)` registers one under an identifier |
| `Scenes/SceneManager.hpp/.cpp` | `SceneManager`: a stack of scenes, the top one is active. `push(id)` builds a scene from its identifier |
| `Scenes/SceneRegistry.hpp/.cpp` | `SceneRegistry`: identifier -> scene factory (`add`, `remove`, `has`, `create`) |
| `Game/GameCatalog.hpp` | `GameCatalog` and `E_GAME`: the games in the program, and how each one prepares the Engine (see below) |
| `Services/Services.hpp` | `Services`: objects owned by the Engine (asset cache, audio...), injected into every scene as `ServiceRef<T>`; read with `getService<T>(registry)` |
| `Components/Uuid.hpp` | `Uuid`: persistent identity of an entity, and its reflection (in the header, so that including it registers the type) |
| `Serialization/` | Saves and loads a world or a reflected value as text: see [Serialization/README.md](Serialization/README.md) |

New `.cpp` files are picked up by CMake automatically. Includes are written
from `src/`: `#include "Engine/Engine.hpp"`.

```sh
cmake --preset all
cmake --build --preset all --target engine_core
```

```cpp
Engine engine;
engine.init();
engine.addService<AssetCache>();                       // outlives the scenes
engine.scenes().registry().add("menu", [] { return std::make_unique<MenuScene>(); });
engine.scenes().push("menu");
engine.run();                                          // or engine.step(dt) from your own loop
// in a system: auto& cache = getService<AssetCache>(registry);
```

## Frame rate

`run()` is the game's main loop. Each frame gets as `dt` the real time since
the previous one.

```cpp
engine.setTargetFrameRate(60);   // the default: run() sleeps until the next frame is due (0: no limit)
engine.setMaxFrameTime(0.25);    // the default: after a long pause, a frame gets at most 0.25 s
```

- 60 frames per second by default. With 0, `run()` runs as fast as it can
  and keeps a core busy. A late frame is not caught up: the schedule starts again.
- Both only apply to `run()`. `step(dt)` never sleeps and passes its `dt` as
  it is: a host with its own loop (the editor) sets its own pace.
- One rate for everything for now. A fixed simulation step (physics at 60 Hz
  while the rendering goes faster) can be added later beside it.

## Scenes made of data: `DataScene`

A `DataScene` is a scene built from a scene file (the format of
[Serialization/README.md](Serialization/README.md)): its entities **and** its
systems are data, made in the editor. The code only defines which systems
exist (`E_SYSTEM`, see the ECS README).

```json
{ "format": "metrov.world", "version": 1,
  "systems": ["InputSystem", "MovementSystem"],
  "entities": [ ... ] }
```

```cpp
addDataScene(engine.scenes().registry(), "level1", "levels/level1.json");
engine.scenes().push("level1");   // false, nothing pushed, if the file cannot be loaded

// with systems of the code, added after the file's ones:
addDataScene(engine.scenes().registry(), "level2", "levels/level2.json", [](DataScene& scene) {
    scene.systems().addSystem<DebugOverlaySystem>(scene.registry());
});
```

It is built in two steps:

1. **Creation** (`DataScene::fromFile` / `fromText`, called by the factory on
   every `push(id)`): the entities are loaded. A file that cannot be loaded
   gives no scene, so `push(id)` returns `false`.
2. **`onEnter()`**: the file's systems are created in order, then the
   optional setup runs. The SceneManager calls it after the Engine injected
   its services, so a system's `init()` sees both the entities and the
   services.

- Reading is tolerant: an unknown system, or one the catalog refuses, is a
  warning and the scene goes on without it; a system whose constructor or
  `init()` throws, or a setup that throws, is an error, and the other
  systems are still created. No exception leaves `onEnter()`, so `push(id)`
  does not throw for them.
- Every error and warning is printed on `std::cerr` as
  `[scene <name>] error|warning: ...` (until the engine has its logger) and
  kept in `scene.loadReport()`.
- Logs are for a human. For the **code** that must act on the result (mark a
  scene as broken in the editor, fail a test, go back to a menu), pass an
  `onReport` handler: it is called once per `push(id)`, with the id and the
  report, at the end of `onEnter()` (load, systems and setup together), or
  right away with the load's errors when `push(id)` fails. It is called even
  when everything went well; an exception it throws is caught.

```cpp
addDataScene(registry, "level1", "levels/level1.json", {}, [](const std::string& id, const serial::Report& report) {
    if (!report.ok()) markBroken(id);
});
```

- `scene.setSystems(list)` replaces the systems of the data: what `save()`
  writes and the next load creates. The editor adds, removes and reorders a
  scene's systems with it. Called before `onEnter()`, it changes which
  systems `onEnter()` creates; after, it does not touch the systems already running;
  empty names are skipped with a warning (in the `Report*` it is given, like
  `save()`, not in `loadReport()`), unknown names are kept.
- Popping then pushing the scene again reads the file again.
- `fromText` builds a scene from text in memory: the editor's Play uses it on
  the text of an open scene (`serial::serialize`), so no file is written.
- `scene.save(file)` writes the scene back: its entities (entities without a
  `Uuid` get one, so saving twice writes the same file) and the systems of
  `scene.info()`, the ones that came from the data. Systems added by the code
  (setup) are not written: the code adds them again on every load, so writing
  them would create them twice.

### Which kind of scene to write

**By default, a scene is a `DataScene` plus systems.** The logic of the game
lives in systems (registered with `E_SYSTEM`), the content in the scene file,
made in the editor. Even generated content fits: a generator system creates
the entities in its `init()`, and keeps its state (seed...) in a component,
so the scene saves and reloads like any other.

Write a `Scene` subclass in C++ only when a system cannot do the job:

- reacting to the scene's own hooks (`onPause`, `onResume`, `onExit`): a
  system is not told about them yet;
- changing how the scene runs (`update()`: fixed time step, slow motion...);
- scenes that are not game content: loading screen, debug tools, test
  scenes. They are registered and pushed like any other scene:

```cpp
engine.scenes().registry().add("physics-test", [] { return std::make_unique<PhysicsTestScene>(); });
engine.scenes().push("physics-test");
```

A C++ scene has no file: the editor can run it (Play goes through the same
`SceneRegistry`) but not edit its content. To turn one into data, run it once
and save its world with `serial::saveFile`, passing
`serial::SceneInfo{scene.systems().names()}` for its systems.

## Games: `E_GAME` and the `GameCatalog`

A game is the code its scene files need: components (`E_REFLECT`), systems
(`E_SYSTEM`), and an **install function** that prepares the Engine for it
(its services: a terminal, an asset cache...). It registers that function
with `E_GAME`, before `main`:

```cpp
namespace demo { inline void install(Engine& engine) { engine.addService<Terminal>(); } }
E_GAME(demo, demo::install)
```

A generic program installs every game it contains, without naming any:

```cpp
GameCatalog::instance().installAll(engine);   // sorted by name; an exception stops it
```

This is what makes `apps/runtime` generic: which games it contains is chosen
in CMake only. It does not matter how a game's code gets into the program:

- **linked** into the executable (today, and for a shipped game: one binary).
  The game must be fully linked, since nothing references its registrations:
  an `OBJECT` library (like `apps/demo`), `WHOLE_ARCHIVE`, or its sources
  compiled into the executable;
- **loaded** from a shared library by a plugin loader (later): its
  registrations run when the library is loaded, and `installAll` is the same.

A name taken twice is not an exception (it would happen before `main`): it is
listed in `GameCatalog::instance().errors()` and printed on `std::cerr`.
