// Wall-clock benchmark for View/Zipper iteration driver selection (#36).
//
// Not part of the unit tests: timings vary between runs and machines, so this
// is built only with -DENGINE_BUILD_BENCHMARKS=ON, and only meaningful in a
// Release (optimized) build.
//
// Build and run commands: see benchmarks/README.md.
// Usage: bench_zipper_driver [repetitions]

#include <cstddef>
#include <cstdint>

#include "BenchSupport.hpp"
#include "ECS/Registry.hpp"

namespace {
    struct Position { int x; int y; };
    struct Velocity { int x; int y; };
    struct Health { int value; };

    // Every query adds what it read into this, so the optimizer can't
    // delete the loop being measured.
    volatile std::int64_t g_sink = 0;

    using Bench::measure;
    using Bench::report;

    template<typename... Components>
    std::size_t iterate(ECS::Registry& registry) {
        std::int64_t sum = 0;
        std::size_t matched = 0;
        for (auto [entity, first, second] : registry.view<Components...>()) {
            (void)entity;
            sum += first.x + second.x;
            ++matched;
        }
        g_sink = g_sink + sum;
        return matched;
    }

    std::size_t iterateThree(ECS::Registry& registry) {
        std::int64_t sum = 0;
        std::size_t matched = 0;
        for (auto [entity, position, health, velocity] :
             registry.view<Position, Health, Velocity>()) {
            (void)entity;
            sum += position.x + health.value + velocity.x;
            ++matched;
        }
        g_sink = g_sink + sum;
        return matched;
    }

    constexpr int Count = 100000;

    // 100,000 entities with Velocity; exactly one of them also has Position.
    void scenarioOneOverlap(int repetitions) {
        ECS::Registry registry;
        for (int i = 0; i < Count; ++i) {
            const auto entity = registry.spawnEntity();
            registry.emplaceComponent<Velocity>(entity, i, i);
            if (i == Count / 2) {
                registry.emplaceComponent<Position>(entity, i, i);
            }
        }

        report("1. 100k Velocity + 1 Position, view<Velocity, Position>",
               measure([&] { return iterate<Velocity, Position>(registry); }, repetitions));
        report("2. 100k Velocity + 1 Position, view<Position, Velocity>",
               measure([&] { return iterate<Position, Velocity>(registry); }, repetitions));
    }

    // 100,000 entities with both components: driver choice cannot save work,
    // so this measures the cost of the selection itself.
    void scenarioFullOverlap(int repetitions) {
        ECS::Registry registry;
        for (int i = 0; i < Count; ++i) {
            const auto entity = registry.spawnEntity();
            registry.emplaceComponent<Velocity>(entity, i, i);
            registry.emplaceComponent<Position>(entity, i, i);
        }

        report("3. 100k Velocity + 100k Position, view<Velocity, Position>",
               measure([&] { return iterate<Velocity, Position>(registry); }, repetitions));
    }

    // Three storages, the smallest one listed in the middle.
    void scenarioThreeStorages(int repetitions) {
        ECS::Registry registry;
        for (int i = 0; i < Count; ++i) {
            const auto entity = registry.spawnEntity();
            registry.emplaceComponent<Position>(entity, i, i);
            registry.emplaceComponent<Velocity>(entity, i, i);
            if (i % 100 == 0) {
                registry.emplaceComponent<Health>(entity, i);
            }
        }

        report("4. 100k Position + 1k Health + 100k Velocity, 3-way",
               measure([&] { return iterateThree(registry); }, repetitions));
    }
}

int main(int argc, char** argv) {
    const int repetitions = Bench::repetitionsFromArgs(argc, argv);

    scenarioOneOverlap(repetitions);
    scenarioFullOverlap(repetitions);
    scenarioThreeStorages(repetitions);
    return 0;
}
