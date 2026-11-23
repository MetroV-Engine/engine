# Logger

The engine-wide logger. Every module (ECS, rendering, GUI, the game loop)
reports through it, so problems can be read per area without scattered
`printf` or `std::cout`. This page explains how to use it and how to test it.
Each function is documented in [Logger.hpp](Logger.hpp), as docstrings.

## Contents

1. [Quick start](#quick-start)
2. [Levels](#levels)
3. [Sections](#sections)
4. [Output and the buffer](#output-and-the-buffer)
5. [Release builds](#release-builds)
6. [Errors](#errors)
7. [How it works](#how-it-works)
8. [Testing the logger](#testing-the-logger)
9. [Known limits](#known-limits)

## Quick start

Each module declares its section once, then logs with `ENGINE_LOG`:

```cpp
#include "Engine/Logger/Logger.hpp"

inline constexpr Log::Section ECS{"ECS"};

void spawnPlayer() {
    ENGINE_LOG(ECS, Log::Level::Info, "player spawned");
}
```

Printed in a Debug build:

```
[Info][ECS] player spawned
```

The message can be a string literal, a `std::string` or a `std::string_view`.
Build any text with values before or inside the call, for example
`ENGINE_LOG(ECS, Log::Level::Debug, "entities: " + std::to_string(count));`.

## Levels

From lowest to highest:

| Level | Use it for | Printed to |
| --- | --- | --- |
| `Debug` | Detail that only matters while developing | stdout |
| `Info` | Normal events worth knowing about | stdout |
| `Success` | A step that finished as expected | stdout |
| `Warning` | Something unexpected, but the engine carries on | stderr |
| `Error` | Something failed | stderr |

## Sections

A section names the module that sent the message, for example `ECS`,
`Renderer` or `GUI`. Declare one per module, once, in a header or at the top of
its `.cpp`:

```cpp
inline constexpr Log::Section Renderer{"Renderer"};
```

Use the same section everywhere in the module, so filtering on its name finds
all of its messages. An empty name is allowed and prints as `[Info][] message`.

## Output and the buffer

- **Output:** each message is printed as one whole line,
  `[Level][Section] message`. Lines from different threads never mix. Each line
  is flushed at once, so the last lines before a crash are not lost when the
  output goes to a pipe or a file (CI logs, IDE consoles).
- **Minimum level:** messages below it are dropped: not built, not stored, not
  printed. The default is `Debug`, so everything is kept.
- **Buffer:** every kept message is also stored in a ring buffer, 1024 entries
  by default. When it is full, the oldest entry is overwritten and counted as
  dropped. A future in-engine console will read this buffer.

```cpp
Log::Logger::setMinLevel(Log::Level::Warning);  // keep only Warning and Error
bool shown = Log::Logger::isEnabled(Log::Level::Info);  // false now
Log::Logger::setCapacity(4096);                 // empties the buffer, new size
auto entries = Log::Logger::snapshot();         // copy, oldest first
std::size_t lost = Log::Logger::droppedCount(); // overwritten since the last clear
Log::Logger::clear();                           // empties the buffer, resets lost
```

- `setCapacity(0)` is treated as `1`.
- `snapshot()` returns a copy, so holding it never blocks logging.
- Each `Entry` holds the level, the section name, the message and a
  `timestamp`: the time since the logger was first used.

## Release builds

Logging exists in Debug builds only. CMake defines `ENGINE_LOG_ENABLED` for
Debug builds of every target that links `engine_logger`, and for no other
build type.

In any other build, `ENGINE_LOG` compiles to nothing: there is no buffer, no
lock, no output, and **its arguments are not evaluated**. So nothing the
program needs may happen inside a log call:

```cpp
// Wrong: in Release, next() never runs.
ENGINE_LOG(ECS, Log::Level::Debug, std::to_string(next()));

// Right: do the work on its own line, then log the result.
const int id = next();
ENGINE_LOG(ECS, Log::Level::Debug, std::to_string(id));
```

The arguments are still type-checked in Release, exactly as in Debug, so a
wrong argument (a typo, or an `int` where text is expected) fails in every
build. A variable used only in a log call does not trigger an "unused variable"
warning.

`ENGINE_LOG` is the way to log. You can also call `Log::Logger::send` directly.
In Release that call does nothing too, but its arguments are still evaluated,
like those of any function, so building the message still costs time.

In Debug builds, a message whose level is below the minimum is not built
either: `ENGINE_LOG` checks the level before it evaluates the message.

The in-engine console will only see messages in Debug builds, because Release
builds have no buffer.

## Errors

Logging never throws. If a message cannot be stored (for example, out of
memory), it is dropped and the buffer is left exactly as it was. If a line
cannot be printed (for example, the output stream fails), the line is dropped
and the message stays in the buffer.

`setCapacity` is a setup call, not a logging call. It throws if the size is
impossible, and the buffer is then left as it was.

You can log from anywhere, including a global object's destructor that runs
after `main` returns: the logger stays alive until the program ends.

## How it works

You do not need this section to use the logger. Read it before you change
`Logger.cpp`, or when you want to know why it behaves the way it does.

### What one `ENGINE_LOG` call does

```
ENGINE_LOG(ECS, Log::Level::Info, "player spawned")
   │
   1. Is Info at or above the minimum level?  ── no ──► stop: the message is never built
   │ yes
   2. Build the entry and the line, with no lock held:
   │     Entry { timestamp, Info, "ECS", "player spawned" }
   │     "[Info][ECS] player spawned\n"
   │
   3. Lock the buffer, move the entry into the next slot, unlock
   │
   4. Lock the output, write the line to stdout or stderr, flush, unlock
```

1. **Level check.** The macro reads the minimum level first. It is a
   `std::atomic`, so this check takes no lock and costs almost nothing.
   If the level is filtered out, the message argument is never evaluated:
   `"x = " + std::to_string(x)` is not even computed.
2. **Build first.** The entry (with its own copies of the section name and
   the message) and the printed line are built before any lock is taken.
   Building is the slow part, so the locks are held only for a moment. And if
   building fails, for example out of memory, nothing has been touched yet.
3. **Store.** Under the buffer lock, the entry is *moved* (not copied) into the
   ring buffer, and the counters are updated.
4. **Print.** Under a second lock, the whole line is written in one call, then
   flushed. Warning and Error go to stderr, the others to stdout.

### The ring buffer

The buffer is a fixed array of slots (1024 by default) used in a circle:

```
capacity 4, after 6 messages:

  slot:     [0]   [1]   [2]   [3]
  holds:    m5    m6    m3    m4        head ──► slot 2 (next write)
                                      count = 4, dropped = 2 (m1, m2)
```

- `head` is the slot the next message goes into. After each message it moves
  one slot forward and wraps back to 0 at the end.
- While the buffer is not full, `count` goes up. Once it is full, each new
  message overwrites the oldest one, and `dropped` goes up instead.
- `snapshot()` starts at the oldest entry and walks forward, so it always
  returns messages oldest first (`m3 m4 m5 m6` above). It returns a copy:
  the console can keep and draw it while other threads keep logging.
- The memory is allocated once. Logging never grows the buffer, so a flood
  of messages cannot use up memory, it only overwrites old ones.

### Threads

Any thread can log at any time. Two locks keep this safe:

| Lock | Protects | Why |
| --- | --- | --- |
| buffer lock | the slots, `head`, `count`, `dropped` | Two threads never write the same slot, and a snapshot never sees a half-written entry. |
| output lock | stdout and stderr | Each line is printed in one piece, so lines from two threads never mix. |

They are separate on purpose: a slow terminal holds only the output lock, so
reading the buffer (`snapshot()`) never waits for printing.

Because storing and printing are two steps, two threads can store in one order
and print in the other. The buffer holds the true order.

### Why logging never throws

A log call must never crash the engine. `send` wraps its whole body in
`try { ... } catch (...) {}`:

- If building the entry fails, step 2 throws before the buffer is touched.
  The message is lost, and the buffer is exactly as before.
- If printing fails, the entry is already stored. Only the printed line is
  lost.
- The counters change only after the entry is in its slot, so they never
  count a message that was not stored.

### Why the logger is never destroyed

The logger's state (buffer, locks, minimum level) is created the first time
anything logs, with `new`, and never deleted:

```cpp
static State* const instance = new State;  // never deleted, on purpose
```

When a C++ program exits, global objects are destroyed in an order you do not
control. If the logger were a normal global, it could be destroyed first, and
a later destructor that logs would write into freed memory. Never deleting it
avoids that. The operating system frees the memory when the program ends.
Valgrind lists it as "still reachable", which is expected and not a leak.

### How Release removes logging

CMake defines `ENGINE_LOG_ENABLED` only in Debug builds, for every target that
links `engine_logger`. `Logger.hpp` then picks one of two versions of the
macro.

Debug (logging on):

```cpp
#define ENGINE_LOG(section, level, message) do {               \
    const ::Log::Level engineLogLevel_ = (level);              \
    if (::Log::Logger::isEnabled(engineLogLevel_)) {           \
        ::Log::Logger::send((section), engineLogLevel_, (message)); \
    } } while (false)
```

Release (logging off):

```cpp
#define ENGINE_LOG(section, level, message) do {               \
    static_cast<void>(sizeof((::Log::Logger::send((section), (level), (message)), 0))); \
    } while (false)
```

- `sizeof` asks the compiler for the size of an expression *without running
  it*. So in Release the compiler still checks that the call to `send` is
  valid (a wrong argument type is an error, as in Debug), but generates no
  code. The arguments are never evaluated.
- `do { ... } while (false)` makes the macro behave like one statement, so it
  is safe after an `if` without braces.
- The library itself has two source files. Debug builds compile
  `Logger.cpp`, the real logger. Every other build compiles
  `LoggerDisabled.cpp` instead, where every function is empty: there is no
  buffer, no lock and no output in the program at all. Each file stops the
  build with an `#error` if it is compiled in the wrong mode.

## Testing the logger

The logger has four test programs and one compile-time check, because it
behaves differently with logging on and off. They live in
[tests/Engine/Logger/](../../../tests/Engine/Logger/), one folder each:

| Folder | Target | Logging | What it checks |
| --- | --- | --- | --- |
| `Enabled/` | `tests_logger` | always on | Storing, filtering, the ring buffer, capacity, output, threads, error handling, `ENGINE_LOG` |
| `Disabled/` | `tests_logger_disabled` | always off | `ENGINE_LOG` evaluates nothing; nothing is stored or printed |
| `Switch/` | `tests_logger_switch` | as the build type sets it | Linking `engine_logger` turns logging on in Debug and off otherwise |
| `Shutdown/` | `tests_logger_shutdown` | always on | Logging from a global destructor after `main`, checked by Valgrind |
| `Disabled/` | `tests_logger_type_check_valid` / `_invalid` | always off | A correct call builds; a call with an `int` message fails to build, as in Debug |

`tests_logger` and `tests_logger_shutdown` compile `Logger.cpp` themselves,
and `tests_logger_disabled` compiles `LoggerDisabled.cpp`, so they test the
same thing in every build type.

### Run the tests

From the repository root, with the `tests` preset (what CI builds):

```sh
cmake --preset tests
cmake --build --preset tests
ctest --preset tests
```

Every test must report `Passed`: the logger's six, plus `tests_ecs` and
`tests_editor`. The two `type_check` tests build a small program: `_invalid`
passes when that build fails.

The preset also builds the editor, which needs SDL3. If vcpkg fails to build
it (on Fedora, install `autoconf-archive` first), test without the editor:

```sh
cmake --preset tests -DENGINE_BUILD_GUI=OFF -DENGINE_BUILD_APP=OFF
cmake --build build/tests
ctest --test-dir build/tests --output-on-failure
```

To run one program on its own, for example while you work on the logger:

```sh
./build/tests/tests_logger
```

It prints a single line, `all tests passed`, and exits with code 0. A failing
check prints `FAILED` with the file, the line and the values compared. With a
multi-config generator such as Visual Studio, the programs are in
`build/tests/Debug/` instead, and `ctest` needs `-C Debug`.

### Run the tests in Release

The logger must also hold up when logging is compiled out. There is no
Release preset, so configure it by hand:

```sh
cmake -S . -B build/release -G Ninja -DCMAKE_BUILD_TYPE=Release -DENGINE_BUILD_TESTS=ON -DENGINE_BUILD_GUI=OFF -DENGINE_BUILD_APP=OFF
cmake --build build/release
ctest --test-dir build/release --output-on-failure
```

### Valgrind and the shutdown test

`tests_logger_shutdown` is run under Valgrind, which reports any read or write
of freed memory. CMake adds it to CTest only when it finds Valgrind. Otherwise
it prints `Valgrind not found: tests_logger_shutdown is built but not run`.
Install it with `sudo dnf install valgrind` (Fedora) or
`sudo apt-get install valgrind` (Ubuntu). CI installs it.

### Coverage

Linux only. Needs `gcovr` and `ninja`:

```sh
python project_scripts/test_coverage.py --minimum 90
```

The script deletes and rebuilds `build/coverage/` with the `coverage` preset,
runs every test and writes an HTML report to
`build/coverage/report/index.html`. It needs a recent `gcovr`, as installed by
`pip install gcovr`; CI does exactly that. A distribution's packaged `gcovr`
can be too old (Fedora's 7.2 rejects the script's `--merge-lines` option).

### Add a test

1. Create a file in `tests/Engine/Logger/Enabled/`, for example
   `TestMyFeature.cpp`, with a `run_logger_my_feature_tests()` function that
   holds the `"name"_test` cases. CMake picks up every `.cpp` in that folder.
2. Start every case with `ScopedLoggerTest scope;`, from
   [LoggerTestSupport.hpp](../../../tests/Engine/Logger/Enabled/LoggerTestSupport.hpp).
   It resets the logger (minimum level `Debug`, capacity 1024, empty buffer)
   and captures stdout and stderr until the case ends. Read what was printed
   with `scope.stdoutText()` and `scope.stderrText()`.
3. Declare the function in `tests/Engine/Logger/Enabled/main.cpp` and call it
   from `main`.
4. Write the test before the code, run it and see it fail, then write the code.

## Known limits

- There is one logger for the whole program, shared by every thread and
  module.
- Timestamps count from the first use of the logger, not from program start.
- Lines from different threads are always whole, but they can print in a
  slightly different order than they were stored.
- Output goes to stdout and stderr only. There are no log files yet.
- `ENGINE_LOG` does not format values itself: build the text, then log it.
