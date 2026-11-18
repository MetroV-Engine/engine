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

1. Add `BenchSomething.cpp` to this directory. Include
   [BenchSupport.hpp](BenchSupport.hpp), read the arguments with
   `Bench::repetitionsFromArgs(argc, argv)` and print every scenario with
   `Bench::report(name, Bench::measure(query, repetitions))`. That is all CI
   needs: `report` also feeds the JSON output.
2. Register it in [CMakeLists.txt](CMakeLists.txt); the name must start with
   `bench_` so the script and CI pick it up:
   ```cmake
   add_executable(bench_something BenchSomething.cpp)
   target_link_libraries(bench_something PRIVATE engine_ecs)
   ```
3. Add a row to the table above, plus a short section.

Keep scenario names stable: the comparison matches scenarios by name, so a
renamed scenario shows up as one *removed* and one *new*, with no comparison
for that run.
