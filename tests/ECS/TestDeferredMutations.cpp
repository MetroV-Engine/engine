#include <boost/ut.hpp>

#include <algorithm>
#include <cstddef>
#include <vector>

#include "ECS/Registry.hpp"

using namespace boost::ut;

namespace {
    struct Position { int x; int y; };

    bool contains(const std::vector<ECS::Entity>& entities, ECS::Entity entity) {
        return std::find(entities.begin(), entities.end(), entity) != entities.end();
    }

    // --- Group A: visibility timing while runSystems() is active ---

    void test_spawn_during_run_systems_is_not_visible_until_flush() {
        "spawnEntity during runSystems is not visible until flush"_test = [] {
            ECS::Registry registry;
            ECS::Entity spawned{};
            bool visibleDuringSystem = true;

            registry.addSystem<Position>([&](ECS::Registry& world) {
                spawned = world.spawnEntity();
                visibleDuringSystem = contains(world.getAllEntities(), spawned);
            });

            registry.runSystems();

            expect(!visibleDuringSystem);
            expect(contains(registry.getAllEntities(), spawned));
        };
    }

    void test_add_component_during_run_systems_is_not_visible_until_flush() {
        "addComponent during runSystems is not visible until flush"_test = [] {
            ECS::Registry registry;
            auto entity = registry.spawnEntity();
            bool hasDuringSystem = false;

            registry.addSystem<Position>([&](ECS::Registry& world) {
                world.addComponent(entity, Position{1, 2});
                hasDuringSystem = world.hasComponent<Position>(entity);
            });

            registry.runSystems();

            expect(!hasDuringSystem);
            expect(registry.hasComponent<Position>(entity));
        };
    }

    void test_remove_component_during_run_systems_is_not_visible_until_flush() {
        "removeComponent during runSystems is not visible until flush"_test = [] {
            ECS::Registry registry;
            auto entity = registry.spawnEntity();
            registry.addComponent(entity, Position{1, 2});
            bool hasDuringSystem = false;

            registry.addSystem<Position>([&](ECS::Registry& world) {
                world.removeComponent<Position>(entity);
                hasDuringSystem = world.hasComponent<Position>(entity);
            });

            registry.runSystems();

            expect(hasDuringSystem);
            expect(!registry.hasComponent<Position>(entity));
        };
    }

    void test_kill_during_run_systems_is_not_visible_until_flush() {
        "killEntity during runSystems is not visible until flush"_test = [] {
            ECS::Registry registry;
            auto entity = registry.spawnEntity();
            bool aliveDuringSystem = false;

            registry.addSystem<Position>([&](ECS::Registry& world) {
                world.killEntity(entity);
                aliveDuringSystem = contains(world.getAllEntities(), entity);
            });

            registry.runSystems();

            expect(aliveDuringSystem);
            expect(!contains(registry.getAllEntities(), entity));
        };
    }

    void test_later_system_in_same_run_systems_call_sees_pre_mutation_state() {
        "a later system in the same runSystems call still sees pre-mutation state"_test = [] {
            ECS::Registry registry;
            ECS::Entity spawned{};
            bool secondSystemSawIt = true;

            registry.addSystem<Position>([&](ECS::Registry& world) {
                spawned = world.spawnEntity();
                world.addComponent(spawned, Position{1, 1});
            });
            registry.addSystem<Position>([&](ECS::Registry& world) {
                secondSystemSawIt = world.hasComponent<Position>(spawned);
            });

            registry.runSystems();

            expect(!secondSystemSawIt);
            expect(registry.hasComponent<Position>(spawned));
        };
    }

    // --- Group A': end-state contracts once flush has happened ---

    void test_spawn_then_add_component_applies_in_order_after_flush() {
        "spawn then addComponent on the same entity applies in order after flush"_test = [] {
            ECS::Registry registry;
            ECS::Entity spawned{};

            registry.addSystem<Position>([&](ECS::Registry& world) {
                spawned = world.spawnEntity();
                world.addComponent(spawned, Position{7, 9});
            });

            registry.runSystems();

            expect(registry.hasComponent<Position>(spawned));
            expect(registry.getComponent<Position>(spawned).x == 7);
            expect(registry.getComponent<Position>(spawned).y == 9);
        };
    }

    void test_mutations_outside_run_systems_remain_immediate() {
        "mutations outside runSystems remain immediate"_test = [] {
            ECS::Registry registry;
            auto entity = registry.spawnEntity();
            expect(contains(registry.getAllEntities(), entity));

            registry.addComponent(entity, Position{3, 4});
            expect(registry.hasComponent<Position>(entity));

            registry.removeComponent<Position>(entity);
            expect(!registry.hasComponent<Position>(entity));

            registry.killEntity(entity);
            expect(!contains(registry.getAllEntities(), entity));
        };
    }

    void test_flushed_queue_does_not_replay_on_next_run_systems() {
        "queued commands do not replay on a later runSystems call"_test = [] {
            ECS::Registry registry;
            int callCount = 0;

            registry.addSystem<Position>([&](ECS::Registry& world) {
                if (callCount == 0) {
                    world.spawnEntity();
                }
                ++callCount;
            });

            registry.runSystems();
            registry.runSystems();

            expect(registry.getAllEntities().size() == std::size_t{1});
        };
    }

    void test_add_component_reference_during_defer_reflects_after_flush() {
        "addComponent's returned reference during deferral is usable and reflects after flush"_test = [] {
            ECS::Registry registry;
            auto entity = registry.spawnEntity();

            registry.addSystem<Position>([&](ECS::Registry& world) {
                auto& position = world.addComponent(entity, Position{1, 1});
                position.x = 99;
            });

            registry.runSystems();

            expect(registry.getComponent<Position>(entity).x == 99);
        };
    }

    // --- Group B: structural mutation must not corrupt an active iteration ---

    void test_remove_component_mid_iteration_does_not_corrupt_view() {
        "removing a component mid-iteration does not corrupt the view"_test = [] {
            ECS::Registry registry;
            std::vector<ECS::Entity> entities;
            for (int i = 0; i < 5; ++i) {
                auto entity = registry.spawnEntity();
                registry.addComponent(entity, Position{i, i});
                entities.push_back(entity);
            }

            std::vector<int> seenX;
            registry.addSystem<Position>([&](ECS::Registry& world) {
                for (auto [entity, position] : world.view<Position>()) {
                    seenX.push_back(position.x);
                    if (entity == entities[1]) {
                        world.removeComponent<Position>(entities[0]);
                    }
                }
            });

            registry.runSystems();

            const std::vector<int> expected{0, 1, 2, 3, 4};
            expect(seenX == expected);
        };
    }

    void test_kill_entity_mid_iteration_does_not_corrupt_view() {
        "killing an entity mid-iteration does not corrupt the view"_test = [] {
            ECS::Registry registry;
            std::vector<ECS::Entity> entities;
            for (int i = 0; i < 5; ++i) {
                auto entity = registry.spawnEntity();
                registry.addComponent(entity, Position{i, i});
                entities.push_back(entity);
            }

            std::vector<int> seenX;
            registry.addSystem<Position>([&](ECS::Registry& world) {
                for (auto [entity, position] : world.view<Position>()) {
                    seenX.push_back(position.x);
                    if (entity == entities[1]) {
                        world.killEntity(entities[0]);
                    }
                }
            });

            registry.runSystems();

            const std::vector<int> expected{0, 1, 2, 3, 4};
            expect(seenX == expected);
        };
    }

    void test_add_component_mid_iteration_does_not_corrupt_view() {
        "adding a component mid-iteration does not corrupt the view"_test = [] {
            ECS::Registry registry;
            std::vector<ECS::Entity> entities;
            for (int i = 0; i < 5; ++i) {
                auto entity = registry.spawnEntity();
                registry.addComponent(entity, Position{i, i});
                entities.push_back(entity);
            }
            auto extra = registry.spawnEntity();

            std::vector<int> seenX;
            registry.addSystem<Position>([&](ECS::Registry& world) {
                for (auto [entity, position] : world.view<Position>()) {
                    seenX.push_back(position.x);
                    if (entity == entities[1]) {
                        world.addComponent(extra, Position{42, 42});
                    }
                }
            });

            registry.runSystems();

            const std::vector<int> expected{0, 1, 2, 3, 4};
            expect(seenX == expected);
            expect(registry.hasComponent<Position>(extra));
        };
    }
}

void run_deferred_mutations_tests() {
    test_spawn_during_run_systems_is_not_visible_until_flush();
    test_add_component_during_run_systems_is_not_visible_until_flush();
    test_remove_component_during_run_systems_is_not_visible_until_flush();
    test_kill_during_run_systems_is_not_visible_until_flush();
    test_later_system_in_same_run_systems_call_sees_pre_mutation_state();
    test_spawn_then_add_component_applies_in_order_after_flush();
    test_mutations_outside_run_systems_remain_immediate();
    test_flushed_queue_does_not_replay_on_next_run_systems();
    test_add_component_reference_during_defer_reflects_after_flush();
    test_remove_component_mid_iteration_does_not_corrupt_view();
    test_kill_entity_mid_iteration_does_not_corrupt_view();
    test_add_component_mid_iteration_does_not_corrupt_view();
}
