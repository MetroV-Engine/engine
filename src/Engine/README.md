# Engine

The runtime library of a game (`engine_core`): the main loop and the scenes.
No editor, no ImGui. The editor (`engine_editor`) and the executables link it.

| File | Content |
| --- | --- |
| `Engine.hpp/.cpp` | `Engine`: `init()`, `run()` (loops until `stop()` or no scene is left), `scenes()` |
| `Scenes/Scene.hpp` | `Scene`: a world (its own `ECS::Registry` and `SystemManager`) with `onEnter` / `onExit` / `onPause` / `onResume` hooks |
| `Scenes/SceneManager.hpp/.cpp` | `SceneManager`: a stack of scenes, the top one is active |

New `.cpp` files are picked up by CMake automatically. Includes are written
from `src/`: `#include "Engine/Engine.hpp"`.

```sh
cmake --preset all
cmake --build --preset all --target engine_core
```
