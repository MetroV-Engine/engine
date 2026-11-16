# Editor

The MetroV Engine editor: a Dear ImGui (docking) interface on top of SDL3,
with a hierarchy, an inspector, a game viewport with Play / Pause / Stop, a
developer console, themes, customizable shortcuts and a saved workspace.

It was developed in the [MetroV-Engine/GUI](https://github.com/MetroV-Engine/GUI)
repository, whose history was merged here. In those commits the files were at
the root of the repository, so browse them from the merge commit's second
parent: `git log <merge commit>^2`.

## Build and run

The editor is optional (it needs SDL3 and Dear ImGui, which CI does not build
yet). Enable it with `ENGINE_BUILD_GUI`; vcpkg (the `vcpkg` submodule:
`git submodule update --init vcpkg`) then installs the `gui` feature of the
manifest:

```sh
cmake --preset all                       # configure (executables and editor), from the engine root
cmake --build --preset all --target editor gui_ecs_example
./build/all/editor                    # editor of an empty ECS world
./build/all/editor --demo             # built-in demo level (no ECS) and widget demo panels
./build/all/gui_ecs_example        # a small game opening the editor on its own world
```

The configuration always starts from the engine root (`CMakePresets.json`);
`--target` builds only what you need. The executables are in `apps/editor`
and `apps/examples`.

The editor writes its layout and preferences (`imgui.ini`, `imgui-demo.ini`),
custom shortcuts (`shortcuts.ini`) and log (`logs/editor.log`) in the working
directory. They are ignored by git.

## Layout

Each class implemented in a `.cpp` lives in a folder of its own name, with
its header: `src/ui/Button/Button.hpp` and `Button.cpp`. Header-only
files (interfaces, templates) stay directly in their module folder, like
`src/scene/HierarchyModel.hpp`. Includes are written from `src/`
(`#include "core/Logger/Logger.hpp"`), except a `.cpp` including its own
header (`#include "Logger.hpp"`). New `.cpp` files are picked up by CMake
automatically.

| Folder | Content |
| --- | --- |
| `src/core/` | `Editor` (assembles everything), `GuiContext` (SDL3 + ImGui, frame loop), `PanelManager`, `EventManager`, `FrameTimer`, `Logger` (central log, thread-safe), `CommandRegistry` (console commands) |
| `src/ui/` | Reusable widgets (buttons, inputs, performance overlay...), dialogs and notifications, themes |
| `src/panels/` | Hierarchy, Inspector, Viewport, Game Controls, Console, Settings, Keyboard Shortcuts... |
| `src/scene/` | What the editor edits and runs: `HierarchyModel` and `PropertyModel` (what the Hierarchy and Inspector display), `Selection`, `SceneBackend`, `Simulation` and `PlayController` (Play / Pause / Stop / Step). `DemoScene` / `DemoBackend`: the built-in demo |
| `src/ecs/` | The editor on an ECS world: `EcsScene` (entities and reflected components), `EcsSimulation` (systems, world restored on Stop), `EcsBackend` |
| `src/navigation/` | Actions, menu bar, toolbar, command palette, shortcuts |
| `src/layout/` | Dockspace layout, status bar, saved workspace |

## Using the editor from the ECS

The editor is the `engine_editor` library (the `editor` executable is a small
`main` around it). A program that has an ECS world, the engine or a game,
links it and gives it its world and systems:

```cmake
target_link_libraries(my_game PRIVATE engine_editor)
```

```cpp
#include "core/Editor/Editor.hpp"
#include "core/GuiContext/GuiContext.hpp"
#include "ECS/ECS.hpp"
#include "ECS/Reflection/Reflection.hpp"

struct Health { int current = 100; int max = 100; };

E_REFLECT(Health)       // what the Inspector can show and add
    E_FIELD(current, int)
    E_FIELD(max, int)
E_END

int main() {
    ECS::Registry world;            // declared first: outlives the editor
    ECS::SystemManager systems;
    // ... systems.addSystem<...>(world), spawn entities ...

    GuiContext gui("My game", 1600, 900);
    Editor editor(gui, world, systems);
    editor.setViewportRenderer(...);   // optional: draw the game with ImGui shapes
    editor.run();
}
```

[examples/EcsExample.cpp](examples/EcsExample.cpp) is a complete example.

What the editor then does with the world:

- **Hierarchy**: every live entity, labelled by its `ECS::Name` (or
  `Entity <index>`). Create spawns a named entity, Delete kills it, Rename
  sets its `ECS::Name`.
- **Inspector**: the entity's components whose type is reflected, read and
  written through `world.inspect()` and `Ref`. `bool`, integers (clamped to
  their range), `float` / `double`, `std::string` and enums (as their integer
  value) are editable; a struct `{float x, y;}` is a Vec2, `{float r, g, b, a;}`
  a Color, any other reflected struct a group of fields. `vector`, `map`,
  `optional` and unreflected fields are shown as a read-only summary. Add
  Component lists every reflected type with a default constructor (the
  `components` console command lists them).
- **Play / Stop**: Play runs `SystemManager::update(world, dt)` every frame.
  Stop restores the world as it was on Play: entities spawned during play are
  killed, the others get their saved components back (added ones are removed,
  removed ones come back), and entities killed during play are spawned again.
  Quitting the editor during play restores it too.

Limits:

- The hierarchy is flat: the ECS has no parent component yet.
- Only reflected, copyable components are saved on Play. Unreflected ones,
  resources and the systems' own state keep their play-time values on Stop.
- An entity killed during play comes back with a new handle: components
  holding its old handle no longer reach it.
- Reflection gives no ranges, formats or enum names, so fields are edited
  with generic widgets.

## Tests

`tests/Editor/` (`tests_editor`, built with `ENGINE_BUILD_GUI`), one folder per
module of `src/`. They run the real editor, headless, in an empty temporary
working directory:

```sh
python project_scripts/test_coverage.py --minimum 90   # build, test, check coverage (Linux)
ctest --preset tests                                    # or only run them
```

`tests/Editor/Support/Headless.hpp` drives the UI the way a user would:

- `Gui`: a real `GuiContext` on SDL's `dummy` video driver and `software`
  renderer, whose frames the test runs one by one (`frame()`).
- Input goes to ImGui directly: `press(ImGuiMod_Ctrl | ImGuiKey_N)`,
  `typeText("help")`, `click(position)`. ImGui sees it on the next frame.
- A widget is clicked by its ImGui id: `activate(itemId(window("Settings"),
  {"VSync"}))`, the activation keyboard navigation uses. `itemId` hashes the
  window and the labels the way ImGui does (a `PushID` scope is one more
  element: `{"Quality", 2, "High"}`); `openPopup(id)` opens a context menu.
- The whole `Editor` runs a `Script`: one step per frame, from the Viewport's
  renderer (`TestEditor.cpp`).
- `LogWatch` checks what was logged since it was created.

Traps met on the way:

- `SDL_Quit` (called by `~GuiContext`) clears every hint: set
  `SDL_HINT_VIDEO_DRIVER` / `SDL_HINT_RENDER_DRIVER` before each `GuiContext`.
- Use the `dummy` video driver with the `software` renderer. `offscreen`
  fails on macOS: SDL asks every window for OpenGL and the driver's EGL does
  not exist there. The dummy display is 1024x768.
- One `GuiContext` at a time: ImGui has one current context.
- The software renderer fills every pixel: a frame of the whole editor takes
  ~300 ms in a debug build. The tests render it at 1/20 of its size
  (`renderSmall()`, through `DisplayFramebufferScale`), ~10 ms; layout and
  input are unchanged.
- A popup must be drawn in the frame it is opened, and focusing another
  window closes it.
