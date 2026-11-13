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
7. [Testing the logger](#testing-the-logger)
8. [Known limits](#known-limits)

## Quick start

Each module declares its section once, then logs with `ENGINE_LOG`:

```cpp
#include "Core/Logger/Logger.hpp"

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
  `[Level][Section] message`. Lines from different threads never mix.
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

The arguments still have to compile in Release, so a typo fails in every build.
A variable used only in a log call does not trigger an "unused variable"
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

## Testing the logger

The logger has four test programs, because it behaves differently with logging
on and off:

| Target | Logging | What it checks |
| --- | --- | --- |
| `tests_logger` | always on | Storing, filtering, the ring buffer, capacity, output, threads, error handling, `ENGINE_LOG` |
| `tests_logger_disabled` | always off | `ENGINE_LOG` evaluates nothing; nothing is stored or printed |
| `tests_logger_switch` | as the build type sets it | Linking `engine_logger` turns logging on in Debug and off otherwise |
| `tests_logger_shutdown` | always on | Logging from a global destructor after `main`, checked by Valgrind |

`tests_logger` and `tests_logger_disabled` compile `Logger.cpp` themselves, so
they test the same thing in every build type.

### Run the tests

From the repository root:

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DENGINE_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

All five test programs, `tests_ecs` included, must report `Passed`. To run one
on its own, for example while you work on the logger:

```sh
./build/tests/tests_logger
```

It prints a single line, `all tests passed`, and exits with code 0. A failing
check prints `FAILED` with the file, the line and the values compared. With a
multi-config generator such as Visual Studio, the programs are in
`build/tests/Debug/` instead.

### Run the tests in Release

The logger must also hold up when logging is compiled out:

```sh
cmake -S . -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release -DENGINE_BUILD_TESTS=ON -DENGINE_BUILD_APP=OFF
cmake --build build-release
ctest --test-dir build-release --output-on-failure
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

The script deletes and rebuilds `build/`, runs every test and writes an HTML
report to `build/tests/coverage/index.html`. It needs a recent `gcovr`, as
installed by `pip install gcovr`; CI does exactly that. A distribution's
packaged `gcovr` can be too old (Fedora's 7.2 rejects the script's
`--merge-lines` option).

### Add a test

1. Create a file in `tests/Logger/`, for example `TestMyFeature.cpp`, with a
   `run_logger_my_feature_tests()` function that holds the `"name"_test`
   cases.
2. Start every case with `ScopedLoggerTest scope;`, from
   [LoggerTestSupport.hpp](../../../tests/Logger/LoggerTestSupport.hpp). It
   resets the logger (minimum level `Debug`, capacity 1024, empty buffer) and
   captures stdout and stderr until the case ends. Read what was printed with
   `scope.stdoutText()` and `scope.stderrText()`.
3. Add the file to `LOGGER_TEST_FILES` in `tests/CMakeLists.txt`.
4. Declare the function in `tests/Logger/main.cpp` and call it from `main`.
5. Write the test before the code, run it and see it fail, then write the code.

## Known limits

- There is one logger for the whole program, shared by every thread and
  module.
- Timestamps count from the first use of the logger, not from program start.
- Lines from different threads are always whole, but they can print in a
  slightly different order than they were stored.
- Output goes to stdout and stderr only. There are no log files yet.
- `ENGINE_LOG` does not format values itself: build the text, then log it.
