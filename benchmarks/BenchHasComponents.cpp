// Wall-clock benchmark for the membership checks and structural mutations
// that read or write entity signatures (#41).
//
// Build and run commands: see benchmarks/README.md.
// Usage: bench_has_components [repetitions]

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <utility>
#include <vector>

#include "BenchSupport.hpp"
#include "ECS/Registry.hpp"

namespace {
    struct Position { int x; int y; };
    struct Velocity { int x; int y; };
    struct Health { int value; };
    template<std::size_t N> struct Extra { int value; };

    using Bench::measure;
    using Bench::report;

    // Every query adds what it counted into this, so the optimizer can't
    // delete the loop being measured.
    volatile std::int64_t g_sink = 0;

    constexpr int Count = 100000;
    constexpr std::size_t ExtraTypes = 1000;

    // Every entity has Position, one in two has Velocity, one in four Health.
    std::vector<ECS::Entity> populate(ECS::Registry& registry) {
        std::vector<ECS::Entity> entities;
        entities.reserve(Count);
        for (int i = 0; i < Count; ++i) {
            const auto entity = registry.spawnEntity();
            registry.emplaceComponent<Position>(entity, i, i);
            if (i % 2 == 0) {
                registry.emplaceComponent<Velocity>(entity, i, i);
            }
            if (i % 4 == 0) {
                registry.emplaceComponent<Health>(entity, i);
            }
            entities.push_back(entity);
        }
        return entities;
    }

    template<typename... Components>
    std::size_t countOwners(const ECS::Registry& registry,
                            const std::vector<ECS::Entity>& entities) {
        std::size_t matched = 0;
        for (const auto entity : entities) {
            if (registry.hasComponents<Components...>(entity)) {
                ++matched;
            }
        }
        g_sink = g_sink + static_cast<std::int64_t>(matched);
        return matched;
    }

    void scenarioHasComponents(int repetitions) {
        ECS::Registry registry;
        const auto entities = populate(registry);

        report("1. 100k hasComponents<Position>",
               measure([&] { return countOwners<Position>(registry, entities); },
                       repetitions));
        report("2. 100k hasComponents<Position, Velocity, Health>",
               measure([&] {
                   return countOwners<Position, Velocity, Health>(registry, entities);
               }, repetitions));
    }

    void scenarioSpawnKill(int repetitions) {
        ECS::Registry registry;
        std::vector<ECS::Entity> entities;
        entities.reserve(Count);

        report("3. spawn then kill 100k entities",
               measure([&] {
                   entities.clear();
                   for (int i = 0; i < Count; ++i) {
                       entities.push_back(registry.spawnEntity());
                   }
                   for (const auto entity : entities) {
                       registry.killEntity(entity);
                   }
                   return entities.size();
               }, repetitions));
    }

    void scenarioAddRemove(int repetitions) {
        ECS::Registry registry;
        std::vector<ECS::Entity> entities;
        entities.reserve(Count);
        for (int i = 0; i < Count; ++i) {
            entities.push_back(registry.spawnEntity());
        }

        report("4. add then remove Health on 100k entities",
               measure([&] {
                   for (const auto entity : entities) {
                       registry.emplaceComponent<Health>(entity, 1);
                   }
                   for (const auto entity : entities) {
                       registry.removeComponent<Health>(entity);
                   }
                   return entities.size();
               }, repetitions));
    }

    // An array of function pointers rather than a fold expression: some
    // compilers cap fold expressions at 256 operands.
    template<std::size_t... Indices>
    void registerExtras(ECS::Registry& registry, std::index_sequence<Indices...>) {
        using Register = void (*)(ECS::Registry&);
        const Register functions[] = {
            [](ECS::Registry& target) { target.registerComponent<Extra<Indices>>(); }...
        };
        for (const Register function : functions) {
            function(registry);
        }
    }

    // Same checks as scenarios 1 and 2, with 1,000 more component types
    // registered, plus a check on the type with the highest id.
    void scenarioManyTypes(int repetitions) {
        using Highest = Extra<ExtraTypes - 1>;

        ECS::Registry registry;
        try {
            registerExtras(registry, std::make_index_sequence<ExtraTypes>{});
        } catch (const std::out_of_range&) {
            std::printf("5-7. skipped: this build cannot register %zu more component types\n",
                        ExtraTypes);
            return;
        }

        const auto entities = populate(registry);
        for (std::size_t i = 0; i < entities.size(); i += 2) {
            registry.emplaceComponent<Highest>(entities[i], 1);
        }

        report("5. +1000 types, 100k hasComponents<Position>",
               measure([&] { return countOwners<Position>(registry, entities); },
                       repetitions));
        report("6. +1000 types, 100k hasComponents<Position, Velocity, Health>",
               measure([&] {
                   return countOwners<Position, Velocity, Health>(registry, entities);
               }, repetitions));
        report("7. +1000 types, 100k hasComponents<highest id>",
               measure([&] { return countOwners<Highest>(registry, entities); },
                       repetitions));
    }
}

int main(int argc, char** argv) {
    const int repetitions = Bench::repetitionsFromArgs(argc, argv);

    scenarioHasComponents(repetitions);
    scenarioSpawnKill(repetitions);
    scenarioAddRemove(repetitions);
    // Keep last: it uses up 1,000 component ids for the whole process.
    scenarioManyTypes(repetitions);
    return 0;
}
