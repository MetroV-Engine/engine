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
component types first, to show that the cost of a check does not depend on
how many types exist.

## Adding a benchmark

1. Add `BenchSomething.cpp` to this directory. Include
   [BenchSupport.hpp](BenchSupport.hpp) for `measure` and `report`.
2. Register it in [CMakeLists.txt](CMakeLists.txt):
   ```cmake
   add_executable(bench_something BenchSomething.cpp)
   target_link_libraries(bench_something PRIVATE engine_ecs)
   ```
3. Add a row to the table above, plus a section with its run command.
