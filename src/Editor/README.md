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
cmake -B build -S . -DENGINE_BUILD_GUI=ON
cmake --build build
./build/gui            # clean editor, empty scene
./build/gui --demo     # demo level and widget demo panels
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
directory. They are ignored by git.

## Layout

| Folder | Content |
| --- | --- |
| `src/` (root) | `GuiContext` (SDL3 + ImGui, frame loop), `Editor` (assembles everything), `EventManager`, `FrameTimer` |
| `src/panels/` | Hierarchy, Inspector, Viewport, Game Controls, Console, Settings, Keyboard Shortcuts... |
| `src/scene/` | `HierarchyModel` and `PropertyModel`: what the Hierarchy and Inspector display. `DemoScene` implements them until the ECS is connected |
| `src/play/` | `PlayController` (Play / Pause / Stop / Step) and the `Simulation` interface it runs |
| `src/navigation/` | Actions, menu bar, toolbar, command palette, shortcuts |
| `src/layout/` | Dockspace layout, status bar, saved workspace |
| `src/log/` | `Logger` (central log, thread-safe) and console commands |
| `src/components/` | Reusable widgets (buttons, inputs...) |
| `src/feedback/`, `src/theme/` | Notifications and dialogs, themes |

## Connecting the ECS

The editor already links `engine_ecs`, but still shows `DemoScene`. The panels
only use interfaces, so connecting the ECS means writing adapters:

- `HierarchyModel` + `PropertyModel` over an `ECS::Registry`, using
  `world.inspect()` (entities, components) and `Ref` / `TypeInfo` (fields);
  property paths such as `"jump.force"` match `Ref::path()`.
- `Simulation::update(dt)` calling `SystemManager::update(world, dt)`, with
  `begin()` / `end()` saving and restoring the world (a `Registry` cannot be
  copied: this needs the reflection layer).

Open points: the ECS has no parent component yet (the hierarchy would be
flat), and the inspector does not show `vector` / `map` / `optional` fields.
