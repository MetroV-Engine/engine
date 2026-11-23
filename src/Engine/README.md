# Engine

The runtime library of a game (`engine_core`): the main loop and the scenes.
No editor, no ImGui. The editor (`engine_editor`) and the executables link it.

| File | Content |
| --- | --- |
| `Engine.hpp/.cpp` | `Engine`: `init()`, `step(dt)` (one frame, for a host with its own loop such as the editor), `run()` (calls `step` until `stop()` or no scene is left), `shutdown()`, `scenes()`, `services()` / `addService<T>()` |
| `Scenes/Scene.hpp` | `Scene`: a world (its own `ECS::Registry` and `SystemManager`) with `onEnter` / `onExit` / `onPause` / `onResume` hooks |
| `Scenes/SceneManager.hpp/.cpp` | `SceneManager`: a stack of scenes, the top one is active. `push(id)` builds a scene from its identifier |
| `Scenes/SceneRegistry.hpp/.cpp` | `SceneRegistry`: identifier -> scene factory (`add`, `remove`, `has`, `create`) |
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
