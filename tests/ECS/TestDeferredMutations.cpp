#include <boost/ut.hpp>

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <vector>

#include "ECS/Registry.hpp"

using namespace boost::ut;

namespace {
    struct Position { int x; int y; };

    bool contains(const std::vector<ECS::Entity>& entities, ECS::Entity entity) {
        return std::find(entities.begin(), entities.end(), entity) != entities.end();
    }

    // --- Group A: visibility timing while runDeferred() is active ---

    void test_spawn_during_run_deferred_is_not_visible_until_flush() {
        "spawnEntity during runDeferred is not visible until flush"_test = [] {
            ECS::Registry registry;
            ECS::Entity spawned{};
            bool visibleDuringDefer = true;

            registry.runDeferred([&] {
                spawned = registry.spawnEntity();
                visibleDuringDefer = contains(registry.getAllEntities(), spawned);
            });

            expect(!visibleDuringDefer);
            expect(contains(registry.getAllEntities(), spawned));
        };
    }

    void test_add_component_during_run_deferred_is_not_visible_until_flush() {
        "addComponent during runDeferred is not visible until flush"_test = [] {
            ECS::Registry registry;
            auto entity = registry.spawnEntity();
            bool hasDuringDefer = false;

            registry.runDeferred([&] {
                registry.addComponent(entity, Position{1, 2});
                hasDuringDefer = registry.hasComponent<Position>(entity);
            });

            expect(!hasDuringDefer);
            expect(registry.hasComponent<Position>(entity));
        };
    }

    void test_remove_component_during_run_deferred_is_not_visible_until_flush() {
        "removeComponent during runDeferred is not visible until flush"_test = [] {
            ECS::Registry registry;
            auto entity = registry.spawnEntity();
            registry.addComponent(entity, Position{1, 2});
            bool hasDuringDefer = false;

            registry.runDeferred([&] {
                registry.removeComponent<Position>(entity);
                hasDuringDefer = registry.hasComponent<Position>(entity);
            });

            expect(hasDuringDefer);
            expect(!registry.hasComponent<Position>(entity));
        };
    }

    void test_kill_during_run_deferred_is_not_visible_until_flush() {
        "killEntity during runDeferred is not visible until flush"_test = [] {
            ECS::Registry registry;
            auto entity = registry.spawnEntity();
            bool aliveDuringDefer = false;

            registry.runDeferred([&] {
                registry.killEntity(entity);
                aliveDuringDefer = contains(registry.getAllEntities(), entity);
            });

            expect(aliveDuringDefer);
            expect(!contains(registry.getAllEntities(), entity));
        };
    }

    void test_later_step_in_same_run_deferred_call_sees_pre_mutation_state() {
        "a later step in the same runDeferred call still sees pre-mutation state"_test = [] {
            ECS::Registry registry;
            ECS::Entity spawned{};
            bool secondStepSawIt = true;

            registry.runDeferred([&] {
                spawned = registry.spawnEntity();
                registry.addComponent(spawned, Position{1, 1});
                secondStepSawIt = registry.hasComponent<Position>(spawned);
            });

            expect(!secondStepSawIt);
            expect(registry.hasComponent<Position>(spawned));
        };
    }

    // --- Group A': end-state contracts once flush has happened ---

    void test_spawn_then_add_component_applies_in_order_after_flush() {
        "spawn then addComponent on the same entity applies in order after flush"_test = [] {
            ECS::Registry registry;
            ECS::Entity spawned{};

            registry.runDeferred([&] {
                spawned = registry.spawnEntity();
                registry.addComponent(spawned, Position{7, 9});
            });

            expect(registry.hasComponent<Position>(spawned));
            expect(registry.getComponent<Position>(spawned).x == 7);
            expect(registry.getComponent<Position>(spawned).y == 9);
        };
    }

    void test_mutations_outside_run_deferred_remain_immediate() {
        "mutations outside runDeferred remain immediate"_test = [] {
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

    void test_flushed_queue_does_not_replay_on_next_run_deferred() {
        "queued commands do not replay on a later runDeferred call"_test = [] {
            ECS::Registry registry;
            int callCount = 0;

            auto step = [&] {
                if (callCount == 0) {
                    registry.spawnEntity();
                }
                ++callCount;
            };

            registry.runDeferred(step);
            registry.runDeferred(step);

            expect(registry.getAllEntities().size() == std::size_t{1});
        };
    }

    void test_add_component_reference_during_defer_reflects_after_flush() {
        "addComponent's returned reference during deferral is usable and reflects after flush"_test = [] {
            ECS::Registry registry;
            auto entity = registry.spawnEntity();

            registry.runDeferred([&] {
                auto& position = registry.addComponent(entity, Position{1, 1});
                position.x = 99;
            });

            expect(registry.getComponent<Position>(entity).x == 99);
        };
    }

    void test_deferring_stops_and_flushes_even_when_callable_throws() {
        "runDeferred turns off deferred mode and flushes even if callable throws"_test = [] {
            ECS::Registry registry;
            ECS::Entity spawned{};

            expect(throws<std::runtime_error>([&] {
                registry.runDeferred([&] {
                    spawned = registry.spawnEntity();
                    throw std::runtime_error("boom");
                });
            }));

            expect(contains(registry.getAllEntities(), spawned));

            auto entity = registry.spawnEntity();
            registry.addComponent(entity, Position{5, 6});
            expect(registry.hasComponent<Position>(entity));
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
            registry.runDeferred([&] {
                for (auto [entity, position] : registry.view<Position>()) {
                    seenX.push_back(position.x);
                    if (entity == entities[1]) {
                        registry.removeComponent<Position>(entities[0]);
                    }
                }
            });

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
            registry.runDeferred([&] {
                for (auto [entity, position] : registry.view<Position>()) {
                    seenX.push_back(position.x);
                    if (entity == entities[1]) {
                        registry.killEntity(entities[0]);
                    }
                }
            });

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
            registry.runDeferred([&] {
                for (auto [entity, position] : registry.view<Position>()) {
                    seenX.push_back(position.x);
                    if (entity == entities[1]) {
                        registry.addComponent(extra, Position{42, 42});
                    }
                }
            });

            const std::vector<int> expected{0, 1, 2, 3, 4};
            expect(seenX == expected);
            expect(registry.hasComponent<Position>(extra));
        };
    }
}

void run_deferred_mutations_tests() {
    test_spawn_during_run_deferred_is_not_visible_until_flush();
    test_add_component_during_run_deferred_is_not_visible_until_flush();
    test_remove_component_during_run_deferred_is_not_visible_until_flush();
    test_kill_during_run_deferred_is_not_visible_until_flush();
    test_later_step_in_same_run_deferred_call_sees_pre_mutation_state();
    test_spawn_then_add_component_applies_in_order_after_flush();
    test_mutations_outside_run_deferred_remain_immediate();
    test_flushed_queue_does_not_replay_on_next_run_deferred();
    test_add_component_reference_during_defer_reflects_after_flush();
    test_deferring_stops_and_flushes_even_when_callable_throws();
    test_remove_component_mid_iteration_does_not_corrupt_view();
    test_kill_entity_mid_iteration_does_not_corrupt_view();
    test_add_component_mid_iteration_does_not_corrupt_view();
}
