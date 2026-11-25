# Demo: a small Sokoban in the terminal

Push every box (`$`) onto a goal (`.`). It shows how the engine runs a game
made of **data** and **systems**, with no game-specific C++ in the engine:

- the **level is a scene file**, [scenes/sokoban.json](scenes/sokoban.json):
  walls, goals, boxes and the player are entities with a `demo::Position`;
- the **logic is systems**, listed by name in the file's `"systems"` block and
  created by the engine through the system catalog (`E_SYSTEM`);
- the **runtime** (`apps/runtime`) knows nothing about Sokoban: it loads the
  file into a `DataScene` and runs `Engine::run()` at 60 frames per second.

```
  #########
  #   #   #
  # $   . #
  #   # $ #
  ##.## @ #
  #       #
  #########
```

`@` the player, `$` a box, `.` a goal, `*` a box on a goal, `+` the player on a goal.

## Prerequisite: curses

curses is needed **by this demo only**, not by the project: it is not a
vcpkg dependency. Without it, CMake prints
`Demo skipped: curses not found` and builds everything else.

| System | Install |
| --- | --- |
| Debian, Ubuntu | `sudo apt install libncurses-dev` |
| Fedora | `sudo dnf install ncurses-devel` |
| Arch | `sudo pacman -S ncurses` |
| macOS | provided by the system (or `brew install ncurses`) |
| Windows (MSVC) | install [PDCurses](https://pdcurses.org/), then give it to CMake: `-DCURSES_INCLUDE_PATH=<dir of curses.h> -DCURSES_LIBRARY=<path of pdcurses.lib>` |

`-DENGINE_BUILD_DEMO=OFF` leaves the demo out even when curses is there.

## Run it

```sh
cmake --preset all
cmake --build --preset all --target runtime
./build/all/runtime apps/demo/scenes/sokoban.json
```

- **Move**: arrow keys, or WASD / ZQSD.
- **Quit**: **Ctrl+C**. The runtime stops the engine after the current frame
  and gives the terminal back as it was.
- `--fps N` changes the frame rate (default 60).

## How it is built

| File | Content |
| --- | --- |
| [Components.hpp](Components.hpp) | `Position`, `Player`, `Box`, `Wall`, `Goal`, `MoveIntent`, reflected (`E_REFLECT`) so the file can describe them |
| [Systems.hpp](Systems.hpp) | `InputSystem` (keys → `MoveIntent`), `MoveSystem` (moves the player, pushes boxes), `ProgressSystem` (boxes on goals → `Progress` resource), `RenderSystem` (draws the grid), registered with `E_SYSTEM` |
| [Terminal.hpp](Terminal.hpp) | The terminal through curses, as a **service**: the only "renderer" and "input" of the demo |
| [Demo.hpp](Demo.hpp) | The game: includes the components and systems, and registers `demo::install(engine)` (adds the terminal service) with `E_GAME` |
| [Demo.cpp](Demo.cpp) | Compiles all of that into the program that links the demo |
| [scenes/sokoban.json](scenes/sokoban.json) | The level |

- **Without a terminal, the systems still run**: `InputSystem` and
  `RenderSystem` do nothing when the `Terminal` service is missing. In the
  editor's Play, the logic will run and the Inspector will show the positions,
  without taking over the terminal.
- **An `OBJECT` library** (`demo_game`): every object file is linked into
  the runtime, none is dropped. Nothing in the runtime references the demo's
  systems or its install function by name, so a static library would lose
  them.
- **The runtime names no game**: it calls `GameCatalog::instance().installAll()`,
  and the demo is in it because CMake links `demo_game` into the runtime.
  Later, the game will be a shared library loaded at run time, and register
  itself the same way.
- **The pace is the engine's**: curses reads keys without waiting, and
  `Engine::run()` sleeps between frames (`setTargetFrameRate`).
