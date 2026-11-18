# Benchmarks

Wall-clock benchmarks for the ECS. They are separate from the unit tests
because their timings depend on the machine and the run.

Benchmarks are built only when `ENGINE_BUILD_BENCHMARKS=ON`. Always use a
`Release` build: timings from a Debug build mean nothing.

## Build

Run from the repository root:

```sh
cmake -S . -B build/bench -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DENGINE_BUILD_BENCHMARKS=ON \
    -DENGINE_BUILD_TESTS=OFF \
    -DENGINE_BUILD_APP=OFF \
    -DENGINE_BUILD_GUI=OFF
cmake --build build/bench --parallel
```

Every benchmark executable ends up at the root of the build directory, for
example `build/bench/bench_zipper_driver`.

## Running

Each benchmark takes the same arguments:

```sh
./build/bench/bench_zipper_driver [repetitions] [--json <file>]
```

- `repetitions` defaults to 200. Raise it (for example to 1000) when results
  are noisy. Each scenario reports its median and minimum time in microseconds.
- `--json <file>` also writes the results to `<file>`, for the comparison
  script and CI (see below). The console output stays the same.

To run all of them at once and compare two runs:

```sh
python project_scripts/benchmarks.py run --out before
# ... change the code, rebuild ...
python project_scripts/benchmarks.py run --out after
python project_scripts/benchmarks.py compare --current after --baseline before
```

`compare` prints a table of the median change per scenario. A scenario is
flagged ⚠️ when it is at least 20% slower (`--threshold` changes this) **and**
at least 1 µs slower: sub-microsecond scenarios swing by tens of percent from
noise alone.

## Available benchmarks

| Executable | Source | Measures |
| --- | --- | --- |
| `bench_zipper_driver` | [BenchZipperDriver.cpp](BenchZipperDriver.cpp) | View/Zipper iteration driven by the smallest storage (#36) |
| `bench_has_components` | [BenchHasComponents.cpp](BenchHasComponents.cpp) | `hasComponents`, spawn/kill and add/remove, which read or write entity signatures (#41) |

### bench_zipper_driver

Four scenarios that check a view is driven by its smallest storage, whatever
the order of its component types.

### bench_has_components

Scenarios 5 to 7 register 1,000 extra component types first, to show what a
check costs when every entity's row of membership bits is much wider.

## Continuous integration

[`.github/workflows/bench.yml`](../.github/workflows/bench.yml) runs on every
pull request to `dev`, every push to `dev`, and by hand (*Actions* tab, *Run
workflow*). It:

1. builds the benchmarks in Release (without vcpkg: they only need
   `engine_ecs`);
2. runs every `bench_*` executable with 1000 repetitions and uploads the JSON
   results as the `bench-results` artifact (kept 90 days);
3. compares them to the results of the **last successful push to `dev`**, the
   baseline, and writes the table to the run's summary page;
4. posts a short digest to Discord: branch, commit, the three largest changes
   and a link to the run.

Regressions are **alerts, not failures**: shared GitHub runners vary from run
to run (and from machine to machine), so the job never fails because of a
timing. When a scenario is flagged, rerun the job or measure locally before
drawing a conclusion. On the very first run, or once the baseline artifact has
expired, there is nothing to compare against and only the raw results are
reported.

The Discord webhook URL is the repository secret `DISCORD_BENCH_WEBHOOK`
(*Settings → Secrets and variables → Actions*). Without it, for example on a
pull request from a fork, the Discord step is skipped and the job still
passes.

## Adding a benchmark

### 1. Write it

Create `BenchSomething.cpp` in this directory. This skeleton is all a new
benchmark needs; CI picks up every scenario passed to `report`:

```cpp
// Wall-clock benchmark for <what it measures> (#<issue>).
// Build and run commands: see benchmarks/README.md.

#include <cstddef>
#include <cstdint>

#include "BenchSupport.hpp"
#include "ECS/ECS.hpp" // the header of the module you measure

namespace {
    // Each query adds what it read into this, so the optimizer cannot delete
    // the code being measured.
    volatile std::int64_t g_sink = 0;

    void scenarioSomething(int repetitions) {
        // Setup goes here, outside the measured lambda.
        ECS::Registry registry;

        // measure() runs the lambda a few times to warm up, then times it
        // `repetitions` times. The lambda returns a count (entities matched,
        // items processed...) printed as "matched" to sanity-check the run.
        Bench::report("1. what this scenario does, with its sizes",
                      Bench::measure([&] {
                          std::size_t count = 0;
                          // ... the work to time, adding to count ...
                          g_sink = g_sink + static_cast<std::int64_t>(count);
                          return count;
                      }, repetitions));
    }
}

int main(int argc, char** argv) {
    // Reads [repetitions] [--json <file>]; --json is what CI uses.
    const int repetitions = Bench::repetitionsFromArgs(argc, argv);

    scenarioSomething(repetitions);
    return 0;
}
```

Guidelines:

- **One scenario, one `report`.** Number them (`1.`, `2.`...) and put the
  sizes in the name (`100k entities`), so the output reads on its own.
- **Keep scenario names stable.** The comparison matches scenarios by name:
  renaming one shows it as *removed* plus *new*, with no comparison for that
  run. Rename only when the scenario really changes.
- **Measure only the work.** Build the data before calling `measure`, and use
  the result (through `g_sink` or the returned count) so the optimizer cannot
  remove it.
- **Aim for at least a few microseconds per scenario.** Below 1 µs, changes are
  never flagged (see [Running](#running)); time a batch of calls instead of a
  single one.

### 2. Register it

Add it to [CMakeLists.txt](CMakeLists.txt). The name **must start with
`bench_`**: that is how `project_scripts/benchmarks.py` and CI find it.

```cmake
add_executable(bench_something BenchSomething.cpp)
target_link_libraries(bench_something PRIVATE engine_ecs)
```

### 3. Document it

Add a row to the [table above](#available-benchmarks), plus a short section
describing its scenarios.

## Benchmarking a new module

Benchmarks are not limited to the ECS. For another module, link its target
instead of (or in addition to) `engine_ecs`, for example `engine_core` for
`src/Engine/` or `engine_render` for `src/Rendering/`, once those libraries
are enabled in their `CMakeLists.txt`.

Check what the CI job can build first. It configures with:

| Setting | Value | Consequence |
| --- | --- | --- |
| vcpkg | not checked out | no vcpkg package is available (no SDL3, no ImGui, no Boost.UT) |
| `ENGINE_BUILD_GUI` | `OFF` | `engine_editor` does not exist |
| `ENGINE_BUILD_APP` / `ENGINE_BUILD_TESTS` | `OFF` | `apps/` and `tests/` are not built |
| Runner | `ubuntu-latest`, no display, no GPU | nothing can open a window or a GPU context |

So a benchmark that needs one of these would build on your machine but break
the **whole** CI job, not just itself. Two ways to handle it:

- **Preferred:** benchmark the part that does not need them (CPU-side code:
  culling, sorting draw calls, physics steps, serialization...).
- **When the dependency is required:** wrap the benchmark in a condition in
  [CMakeLists.txt](CMakeLists.txt) so it is only built where the dependency
  exists, and say in its README section that CI does not run it:
  ```cmake
  if(ENGINE_BUILD_GUI)
    add_executable(bench_something_gui BenchSomethingGui.cpp)
    target_link_libraries(bench_something_gui PRIVATE engine_editor)
  endif()
  ```
  Having CI run it would mean changing [bench.yml](../.github/workflows/bench.yml)
  (vcpkg, apt packages, a virtual display) like `build.yml` does: a separate
  change, since it makes the job much slower.

Keep a full run short too: CI runs every benchmark with 1000 repetitions. The
current ones take about 7 s in total at that count; a new benchmark should
add no more than a few seconds.
