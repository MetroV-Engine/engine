# Benchmarks

Wall-clock benchmarks for the ECS. They are separate from the unit tests
because their timings depend on the machine and the run.

Benchmarks are built only when `ENGINE_BUILD_BENCHMARKS=ON`. Always use a
`Release` build: timings from a Debug build mean nothing.

## Build

Run from the repository root:

```sh
cmake -S . -B cmake-build-bench -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DENGINE_BUILD_BENCHMARKS=ON \
    -DENGINE_BUILD_TESTS=OFF \
    -DENGINE_BUILD_APP=OFF
cmake --build cmake-build-bench --parallel
```

Every benchmark executable ends up in `cmake-build-bench/benchmarks/`.

## Available benchmarks

| Executable | Source | Measures |
| --- | --- | --- |
| `bench_zipper_driver` | [BenchZipperDriver.cpp](BenchZipperDriver.cpp) | View/Zipper iteration driven by the smallest storage (#36) |
| `bench_has_components` | [BenchHasComponents.cpp](BenchHasComponents.cpp) | `hasComponents`, spawn/kill and add/remove, which read or write entity signatures (#41) |
| `bench_collision_recentering` | [BenchCollisionRecentering.cpp](BenchCollisionRecentering.cpp) | Precision and cost of `sweep(circle, rectangle)` computed in world space or around (0, 0), from 0 m to 100 km from the origin (#65) |

### bench_zipper_driver

```sh
./cmake-build-bench/benchmarks/bench_zipper_driver [repetitions]
```

`repetitions` defaults to 200. Raise it (for example to 1000) when results are
noisy. The benchmark reports the median and the minimum time in microseconds
for each scenario.

To keep a result so you can compare it later:

```sh
./cmake-build-bench/benchmarks/bench_zipper_driver > zipper_driver_before.txt
```

### bench_has_components

```sh
./cmake-build-bench/benchmarks/bench_has_components [repetitions]
```

`repetitions` defaults to 200. Scenarios 5 to 7 register 1,000 extra
component types first, to show what a check costs when every entity's row
of membership bits is much wider.

### bench_collision_recentering

```sh
taskset -c 2 ./cmake-build-bench/benchmarks/bench_collision_recentering [repetitions] [all|precision|cost]
```

Compares three versions of `sweep(circle, rectangle)`: the former code, the
current structure in world space (the control: only the frame differs), and
the current code, which works around (0, 0).

- `precision` runs the same 1,000,000 seeded cases at 0 m, 100 m, 1 km,
  10 km and 100 km from the origin. It does not depend on `repetitions` and
  gives the same numbers on every run and with every compiler.
- `cost` times the three versions back to back in each repetition (paired, in
  rotating order), and reports the median overheads with their quartiles.
  `repetitions` defaults to 200.
- `taskset` pins the run to one core. The program ends with a PASS/FAIL line
  for each success criterion.

Timings of a function this small also depend on where the compiler places its
code. To check that a result does not, rebuild with a few paddings and compare:

```sh
for pad in 0 32 64 96; do
    g++ -std=gnu++20 -O3 -DNDEBUG -DBENCH_LAYOUT_PADDING=$pad -Isrc -Ibenchmarks \
        benchmarks/BenchCollisionRecentering.cpp -o bench_pad_$pad
    taskset -c 2 ./bench_pad_$pad 200 cost
done
```

## Adding a benchmark

1. Add `BenchSomething.cpp` to this directory. Include
   [BenchSupport.hpp](BenchSupport.hpp) for `measure` and `report`.
2. Register it in [CMakeLists.txt](CMakeLists.txt):
   ```cmake
   add_executable(bench_something BenchSomething.cpp)
   target_link_libraries(bench_something PRIVATE engine_ecs)
   ```
3. Add a row to the table above, plus a section with its run command.
