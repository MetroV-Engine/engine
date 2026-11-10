# Editor

The MetroV Engine editor: a Dear ImGui (docking) interface on top of SDL3,
with a hierarchy, an inspector, a palette of entities to drag into the scene,
a game viewport with Play / Pause / Stop, a developer console, themes,
customizable shortcuts and a saved workspace.

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
cmake -B build -S . -DENGINE_BUILD_GUI=ON
cmake --build build
./build/gui               # editor of an empty ECS world
./build/gui --demo        # built-in demo level (no ECS) and widget demo panels
./build/gui_ecs_example   # a small game opening the editor on its own world
```

When working only on the editor, it can also be built from its own folder
(it adds the ECS itself and uses the same manifest):

```sh
cd src/Editor
cmake -S . -B build
cmake --build build
./build/gui --demo
```

The editor writes its layout and preferences (`imgui.ini`, `imgui-demo.ini`),
custom shortcuts (`shortcuts.ini`) and log (`logs/editor.log`) in the working
directory. They are ignored by git. Levels are saved there too, in `levels/`
(see [Levels](#levels)): they are not ignored. The Asset Browser reads `assets/`
there (see [Assets](#assets)).

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
| `src/core/` | `Editor` (assembles everything), `AssetLibrary` (the files of the assets folder), `GuiContext` (SDL3 + ImGui, frame loop), `PanelManager`, `EventManager`, `FrameTimer`, `Logger` (central log, thread-safe), `CommandRegistry` (console commands) |
| `src/ui/` | Reusable widgets (buttons, inputs, performance overlay...), dialogs and notifications, themes |
| `src/panels/` | Hierarchy, Entity Palette, Inspector, Viewport, Game Controls, Console, Assets, Settings, Levels, Keyboard Shortcuts... `EntityDropTarget` creates the entities dropped on a panel, `ViewportTransformTool` selects, moves and scales objects with the mouse |
| `src/scene/` | What the editor edits and runs: `HierarchyModel` and `PropertyModel` (what the Hierarchy and Inspector display), `Selection`, `SceneBackend`, `EntityTemplate` (an entry of the Entity Palette), `ViewportView` (Viewport pixels to world positions), `ViewportGrid` (snapping of the mouse edits), `LevelFile` and `LevelManager` (levels saved as files), `Simulation` and `PlayController` (Play / Pause / Stop / Step). `DemoScene` / `DemoBackend`: the built-in demo |
| `src/ecs/` | The editor on an ECS world: `EcsScene` (entities and reflected components), `EcsSimulation` (systems, world restored on Stop), `EcsBackend` |
| `src/navigation/` | Actions, menu bar, toolbar, command palette, shortcuts |
| `src/layout/` | Dockspace layout, status bar, saved workspace |
| `examples/` | `EcsExample.cpp`: a game using the editor (`gui_ecs_example`) |
| `tests/` | `tests_editor`: the editor on an ECS world (see [Tests](#tests)) |

## Using the editor from the ECS

The editor is the `engine_editor` library (the `gui` executable is a small
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
    editor.addEntityTemplate({"Mob", "Mob", {"Health"}});   // optional: label, name, components
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
- **Entity Palette**: drag a template onto the Hierarchy or the Viewport to
  create its entity and select it. An ECS world starts with `Empty Entity`;
  `addEntityTemplate` adds more. A component that cannot be added gives a
  warning and the entity is created without it. A layout saved before this
  panel existed gets it docked below the Hierarchy, like any panel a saved
  layout does not know is docked in its default area.
- **Viewport**: a template dropped there lands under the mouse. Clicking near
  an object selects it (the Hierarchy scrolls to it) and dragging moves it;
  the handles of the selected object scale it (see
  [Editing objects with the mouse](#editing-objects-with-the-mouse)).
  Clicking empty space clears the selection.
- **During Play**: nothing can be created (Entity Palette, Hierarchy `+` and
  menus, Edit > Create Object), a drag in progress is cancelled and objects
  cannot be moved or scaled with the mouse: created during play, objects
  would vanish on Stop.
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

### Editing objects with the mouse

The renderer given to `setViewportRenderer` returns how it mapped the world to
the screen, as a `ViewportView`: the screen position of the world's (0, 0) and
the pixels per world unit.

```cpp
editor.setViewportRenderer([&world](ImDrawList *drawList, ImVec2 min, ImVec2 max) {
    const ViewportView view{min, 2.0f};
    // ... draw each object at view.toScreen(Vec2{x, y}) ...
    return view;
});
```

The mouse reads and writes two fields of the `Transform` component: `position`
to place and move objects, `scale` to resize them. A game needs a reflected
component with these names, both fields being a struct `{float x, y;}`; an
object without a `scale` can be moved but not resized. A click grabs the
nearest object within 24 pixels.

The selected object shows three scale handles: one stretches it horizontally,
one vertically, the corner one on both axes, keeping their ratio. The renderer
decides what the scale looks like: the demo multiplies its boxes by it,
`EcsExample.cpp` its circles.

Settings > Viewport turns snapping on: positions, dropped templates included,
snap to a grid of world units (16 by default) drawn under the game, and scales
to tenths. Both settings are saved with the workspace.

Without a view (`ViewportView{}`, or no renderer as in `./build/gui`) the mouse
cannot select or move anything in the Viewport. A template dropped there, or
on the Hierarchy, keeps its default position.

### Levels

A level is the scene saved in `levels/<name>.level`, in the working
directory. The Levels panel (next to the Inspector) shows the open level, with
Save, Save As and New, and the levels folder: double click opens a level,
right click renames or deletes it. Names use letters, digits, `-` and `_`.
File > Save Level (Ctrl+S) saves the open level, or asks for a name. The
`level` console command does the same: `level list`, `level new <name>`,
`level open <name>`, `level save [new name]`, `level rename <old> <new>`,
`level delete <name>`.

Unsaved changes are only discarded after a confirmation (the console command
refuses instead), and levels cannot change during Play.

The file is text, written by `LevelFile` through the `HierarchyModel` and
`PropertyModel`, so the demo scene and an ECS world both use it: each object
with its name, parent and components, with their editable properties.

```
metrov-level 1
object 0 - "Player"
component "Transform"
property "position" vec2 64 160
```

Not saved: read-only fields (`vector`, `map`...), unreflected components,
resources and the systems' own state. On an ECS world, an unnamed entity is
saved under the name the Hierarchy shows (`Entity <index>`), and parents are
ignored (the hierarchy is flat). This format belongs to the editor until the
engine has its own scene format.

### Assets

The Assets panel (next to the Console) shows the files of `assets/`, in the
working directory, as a tree of folders, or the matching files while
searching. The selected file shows its type (from its extension: image, audio,
font, level, text) and size, with a button to copy its path. Dragging a file
onto a text field of the Inspector (a sprite's texture...) writes its path,
relative to `assets/`, there. The folder is read again every two seconds
while the panel is shown; hidden files are skipped, and a scan stops at 10000
files. The `assets` console command counts them.

Images are not previewed: the editor has no image loader yet.

## Tests

`tests_editor` checks that the editor works on an ECS world, with the
engine's test framework (boost-ext ut). It is built when the editor is built
from its own folder, or from the engine root with the engine's tests:

```sh
cd src/Editor && cmake -S . -B build && cmake --build build
ctest --test-dir build --output-on-failure

# or from the engine root, with tests_ecs
cmake -B build -S . -DENGINE_BUILD_GUI=ON -DENGINE_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

| File | What it checks |
| --- | --- |
| `tests/ecs/TestEcsScene.cpp` | Hierarchy and Inspector: entities listed, created, renamed and killed; ids of dead entities never reused; each field type gets its widget; values read and written in the world (integers clamped); components added and removed |
| `tests/ecs/TestEcsSimulation.cpp` | Play runs the systems; Stop restores values and names, removes added components, gives back removed ones, kills entities spawned in play, spawns killed ones again; non-copyable components kept with a warning |
| `tests/ecs/TestEcsBackend.cpp` | What the Editor receives: the three parts work on the same world, description, `components` command |
| `tests/ecs/TestEditorOnEcs.cpp` | The real Editor in an offscreen window: selects an entity, plays, pauses, steps, stops, quits (also during play); the world is restored and no error is logged. A panel missing from a saved layout is docked in its area; Create Object does nothing during Play, even from a shortcut; an object selected outside the Hierarchy is scrolled to there; the Game Controls keep their place in every play state and fit the smallest window; levels are saved, created, opened and renamed from the console, never during Play nor over unsaved changes; the assets folder is read and the Asset Browser shows it |
| `tests/ecs/TestLevels.cpp` | A level written and read back gives the same ECS world and the same demo scene (order, parents, every value type, escaped strings); an invalid file leaves the scene untouched; what the scene refuses is reported; the levels folder: create, save, save as, open, rename, delete, unsaved changes, refused names and files |
| `tests/ecs/TestAssets.cpp` | The assets folder: files of every subfolder, sorted, with their type (extensions in any case) and size; hidden files and folders skipped; a missing folder; a new scan sees new files |
| `tests/ecs/TestEntityDragDrop.cpp` | The real Editor with a simulated mouse: a template dropped on the Hierarchy or the Viewport creates and selects its entity (under the mouse on the Viewport), released elsewhere it creates nothing; objects are selected and moved, and scaled with the handles of the selected one, and clicking empty space clears the selection; with snapping on, moves, drops and scales land on the grid, and the settings are saved; nothing is created, moved or scaled during Play, even when Play is pressed during the drag or at the drop |

On macOS SDL's `offscreen` driver cannot start, so the tests opening the real
Editor fail under plain `ctest`. Run them with
`SDL_VIDEO_DRIVER=dummy SDL_RENDER_DRIVER=software ./build/tests_editor`
(`./build/src/Editor/tests_editor` from the engine root).

The reflection registry is shared by the whole test program: each file
prefixes its test components (`SceneMover`, `SimPosition`...) so their names
never collide. `SDL_VIDEO_DRIVER=x11 ./build/tests_editor` (or `wayland`)
shows the editor window during the end-to-end test.

The editor is not built in CI yet (SDL3 and Dear ImGui), so these tests run
locally only.
