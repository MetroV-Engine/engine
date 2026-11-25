#include <boost/ut.hpp>

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "ECS/Core/System/SystemManager.hpp"

using namespace boost::ut;

namespace {
    struct Position { int x; int y; };
    struct Velocity { int x; int y; };


    class OrderRecordingSystem : public ECS::ISystem {
        public:
            OrderRecordingSystem(std::vector<int>& order, int id) : _order(order), _id(id) {}

            void update(ECS::Registry&, double) override {
                _order.push_back(_id);
            }

            const std::string name() const override { return "OrderRecordingSystem"; }

        private:
            std::vector<int>& _order;
            int _id;
    };

    class DtRecordingSystem : public ECS::ISystem {
        public:
            explicit DtRecordingSystem(double& lastDt) : _lastDt(lastDt) {}

            void update(ECS::Registry&, double dt) override {
                _lastDt = dt;
            }

            const std::string name() const override { return "DtRecordingSystem"; }

        private:
            double& _lastDt;
    };

    class MovementSystem : public ECS::ISystem {
        public:
            void update(ECS::Registry& reg, double dt) override {
                for (auto [entity, position, velocity] : reg.view<Position, Velocity>()) {
                    (void)entity;
                    position.x += static_cast<int>(velocity.x * dt);
                    position.y += static_cast<int>(velocity.y * dt);
                }
            }

            const std::string name() const override { return "MovementSystem"; }
    };

    class SpawningSystem : public ECS::ISystem {
        public:
            SpawningSystem(ECS::Entity& spawned, bool& visibleDuringUpdate)
                : _spawned(spawned), _visibleDuringUpdate(visibleDuringUpdate) {}

            void update(ECS::Registry& reg, double) override {
                _spawned = reg.spawnEntity();
                _visibleDuringUpdate = reg.isAlive(_spawned);
            }

            const std::string name() const override { return "SpawningSystem"; }

        private:
            ECS::Entity& _spawned;
            bool& _visibleDuringUpdate;
    };

    class EntitySpawningSystem : public ECS::ISystem {
        public:
            void update(ECS::Registry& reg, double) override {
                reg.spawnEntity();
            }

            const std::string name() const override { return "EntitySpawningSystem"; }
    };

    class EntityCountRecordingSystem : public ECS::ISystem {
        public:
            explicit EntityCountRecordingSystem(std::size_t& countSeen) : _countSeen(countSeen) {}

            void update(ECS::Registry& reg, double) override {
                _countSeen = reg.entityCount();
            }

            const std::string name() const override { return "EntityCountRecordingSystem"; }

        private:
            std::size_t& _countSeen;
    };

    class LifecycleRecordingSystem : public ECS::ISystem {
        public:
            LifecycleRecordingSystem(int& initCount, int& shutdownCount)
                : _initCount(initCount), _shutdownCount(shutdownCount) {}

            void init(ECS::Registry&) override { ++_initCount; }
            void shutdown(ECS::Registry&) override { ++_shutdownCount; }

            void update(ECS::Registry&, double) override {}

            const std::string name() const override { return "LifecycleRecordingSystem"; }

        private:
            int& _initCount;
            int& _shutdownCount;
    };

    void test_add_system_returns_reference_to_constructed_instance() {
        "addSystem returns a reference to the constructed instance"_test = [] {
            ECS::Registry registry;
            double lastDt = -1.0;
            ECS::SystemManager systems;
            auto& system = systems.addSystem<DtRecordingSystem>(registry, lastDt);

            expect(system.name() == std::string("DtRecordingSystem"));
        };
    }

    void test_size_reflects_registered_systems() {
        "size reflects the number of registered systems"_test = [] {
            ECS::Registry registry;
            ECS::SystemManager systems;
            expect(systems.size() == std::size_t{0});

            double dtA = 0.0;
            double dtB = 0.0;
            systems.addSystem<DtRecordingSystem>(registry, dtA);
            expect(systems.size() == std::size_t{1});

            systems.addSystem<DtRecordingSystem>(registry, dtB);
            expect(systems.size() == std::size_t{2});
        };
    }

    void test_update_runs_systems_in_registration_order() {
        "update runs systems in registration order"_test = [] {
            ECS::Registry registry;
            ECS::SystemManager systems;
            std::vector<int> order;
            systems.addSystem<OrderRecordingSystem>(registry, order, 1);
            systems.addSystem<OrderRecordingSystem>(registry, order, 2);
            systems.addSystem<OrderRecordingSystem>(registry, order, 3);

            systems.update(registry, 0.0);

            expect(order == std::vector<int>{1, 2, 3});
        };
    }

    void test_update_passes_dt_to_systems() {
        "update passes dt through to each system"_test = [] {
            ECS::Registry registry;
            ECS::SystemManager systems;
            double lastDt = -1.0;
            systems.addSystem<DtRecordingSystem>(registry, lastDt);

            systems.update(registry, 0.5);

            expect(lastDt == 0.5);
        };
    }

    void test_update_runs_a_system_over_a_never_touched_component() {
        "update runs a system querying a component no entity has yet"_test = [] {
            ECS::Registry registry;
            auto entity = registry.spawnEntity();
            registry.emplaceComponent<Velocity>(entity, 2, 3);
            // Position is never emplaced: MovementSystem's view<Position, Velocity>()
            // must not throw even though no entity has a Position yet.

            ECS::SystemManager systems;
            systems.addSystem<MovementSystem>(registry);

            systems.update(registry, 1.0);

            expect(!registry.hasComponents<Position>(entity));
        };
    }

    void test_update_moves_entities_matching_the_view() {
        "update moves entities that have both Position and Velocity"_test = [] {
            ECS::Registry registry;
            auto entity = registry.spawnEntity();
            registry.emplaceComponent<Position>(entity, 0, 0);
            registry.emplaceComponent<Velocity>(entity, 2, 3);

            ECS::SystemManager systems;
            systems.addSystem<MovementSystem>(registry);

            systems.update(registry, 2.0);

            expect(registry.getComponent<Position>(entity).x == 4);
            expect(registry.getComponent<Position>(entity).y == 6);
        };
    }

    void test_update_runs_under_deferred_mutation_mode() {
        "update runs systems under deferred-mutation mode"_test = [] {
            ECS::Registry registry;
            ECS::SystemManager systems;
            ECS::Entity spawned{};
            bool visibleDuringUpdate = true;
            systems.addSystem<SpawningSystem>(registry, spawned, visibleDuringUpdate);

            systems.update(registry, 0.0);

            expect(!visibleDuringUpdate);
            expect(registry.isAlive(spawned));
        };
    }

    void test_update_flushes_between_systems_for_same_frame_visibility() {
        "update flushes deferred commands between systems, not just at frame end"_test = [] {
            ECS::Registry registry;
            ECS::SystemManager systems;
            std::size_t countSeenBySecondSystem = 0;
            systems.addSystem<EntitySpawningSystem>(registry);
            systems.addSystem<EntityCountRecordingSystem>(registry, countSeenBySecondSystem);

            systems.update(registry, 0.0);

            // EntitySpawningSystem spawns one entity; EntityCountRecordingSystem
            // runs right after in the same frame and must already see it,
            // instead of waiting for the next update() call.
            expect(countSeenBySecondSystem == std::size_t{1});
        };
    }

    void test_add_system_calls_init_once() {
        "addSystem calls init on the new system exactly once"_test = [] {
            ECS::Registry registry;
            ECS::SystemManager systems;
            int initCount = 0;
            int shutdownCount = 0;

            systems.addSystem<LifecycleRecordingSystem>(registry, initCount, shutdownCount);

            expect(initCount == 1);
            expect(shutdownCount == 0);
        };
    }

    void test_remove_system_calls_shutdown_once_and_removes_it() {
        "removeSystem calls shutdown once, removes the system and getSystem no longer finds it"_test = [] {
            ECS::Registry registry;
            ECS::SystemManager systems;
            int initCount = 0;
            int shutdownCount = 0;
            systems.addSystem<LifecycleRecordingSystem>(registry, initCount, shutdownCount);

            expect(systems.getSystem<LifecycleRecordingSystem>() != nullptr);

            const bool removed = systems.removeSystem<LifecycleRecordingSystem>(registry);

            expect(removed);
            expect(initCount == 1);
            expect(shutdownCount == 1);
            expect(systems.getSystem<LifecycleRecordingSystem>() == nullptr);
            expect(systems.size() == std::size_t{0});
        };
    }

    void test_remove_system_returns_false_when_not_found() {
        "removeSystem returns false and does nothing when no matching system is registered"_test = [] {
            ECS::Registry registry;
            ECS::SystemManager systems;
            double lastDt = -1.0;
            systems.addSystem<DtRecordingSystem>(registry, lastDt);

            const bool removed = systems.removeSystem<MovementSystem>(registry);

            expect(!removed);
            expect(systems.size() == std::size_t{1});
        };
    }

    // ---- addSystem(reg, unique_ptr): a system created elsewhere (by name)

    void test_add_created_system_takes_it_and_calls_init() {
        "addSystem of a created system takes ownership, calls init once and returns it"_test = [] {
            ECS::Registry registry;
            ECS::SystemManager systems;
            int initCount = 0;
            int shutdownCount = 0;
            auto created = std::make_unique<LifecycleRecordingSystem>(initCount, shutdownCount);
            const ECS::ISystem* address = created.get();

            ECS::ISystem& added = systems.addSystem(registry, std::move(created));

            expect(&added == address);
            expect(initCount == 1);
            expect(shutdownCount == 0);
            expect(systems.size() == std::size_t{1});
        };
    }

    void test_add_created_system_runs_after_existing_ones() {
        "a created system runs after the systems already added, in order"_test = [] {
            ECS::Registry registry;
            ECS::SystemManager systems;
            std::vector<int> order;
            systems.addSystem<OrderRecordingSystem>(registry, order, 1);
            systems.addSystem(registry, std::make_unique<OrderRecordingSystem>(order, 2));
            systems.addSystem<OrderRecordingSystem>(registry, order, 3);

            systems.update(registry);

            expect(order == std::vector<int>{1, 2, 3});
        };
    }

    void test_add_created_system_is_found_and_removed_by_type() {
        "a created system is found and removed by its concrete type"_test = [] {
            ECS::Registry registry;
            ECS::SystemManager systems;
            int initCount = 0;
            int shutdownCount = 0;
            systems.addSystem(registry, std::make_unique<LifecycleRecordingSystem>(initCount, shutdownCount));

            expect(systems.getSystem<LifecycleRecordingSystem>() != nullptr);
            expect(systems.removeSystem<LifecycleRecordingSystem>(registry));
            expect(shutdownCount == 1);
            expect(systems.size() == std::size_t{0});
        };
    }

    // ---- getSystem(name) / removeSystem(reg, name): the editor only knows names

    // A system named at construction, recording its runs and its shutdowns.
    class NamedSystem : public ECS::ISystem {
        public:
            NamedSystem(std::string name, std::vector<std::string>& runs, int& shutdowns)
                : _name(std::move(name)), _runs(runs), _shutdowns(shutdowns) {}

            void update(ECS::Registry&, double) override { _runs.push_back(_name); }
            void shutdown(ECS::Registry&) override { ++_shutdowns; }
            const std::string name() const override { return _name; }

        private:
            std::string _name;
            std::vector<std::string>& _runs;
            int& _shutdowns;
    };

    void test_get_system_by_name_finds_it() {
        "getSystem(name) returns the system with that name"_test = [] {
            ECS::Registry registry;
            ECS::SystemManager systems;
            std::vector<std::string> runs;
            int shutdowns = 0;
            systems.addSystem<NamedSystem>(registry, "First", runs, shutdowns);
            auto& second = systems.addSystem<NamedSystem>(registry, "Second", runs, shutdowns);

            expect(systems.getSystem("Second") == &second);
        };
    }

    void test_get_system_by_name_unknown_or_empty() {
        "getSystem(name) returns nullptr for an unknown or empty name"_test = [] {
            ECS::Registry registry;
            ECS::SystemManager systems;
            std::vector<std::string> runs;
            int shutdowns = 0;
            expect(systems.getSystem("Anything") == nullptr);   // no system at all

            systems.addSystem<NamedSystem>(registry, "First", runs, shutdowns);
            expect(systems.getSystem("Unknown") == nullptr);
            expect(systems.getSystem("") == nullptr);
            expect(systems.getSystem("first") == nullptr);   // case sensitive
        };
    }

    void test_get_system_by_name_returns_the_first_of_two() {
        "getSystem(name) returns the first of two systems with the same name"_test = [] {
            ECS::Registry registry;
            ECS::SystemManager systems;
            std::vector<std::string> runs;
            int shutdowns = 0;
            auto& first = systems.addSystem<NamedSystem>(registry, "Twin", runs, shutdowns);
            systems.addSystem<NamedSystem>(registry, "Twin", runs, shutdowns);

            expect(systems.getSystem("Twin") == &first);
        };
    }

    void test_remove_system_by_name_removes_it() {
        "removeSystem(name) calls shutdown once and removes the system"_test = [] {
            ECS::Registry registry;
            ECS::SystemManager systems;
            std::vector<std::string> runs;
            int shutdowns = 0;
            systems.addSystem<NamedSystem>(registry, "Gone", runs, shutdowns);

            expect(systems.removeSystem(registry, "Gone"));

            expect(shutdowns == 1);
            expect(systems.size() == std::size_t{0});
            expect(systems.getSystem("Gone") == nullptr);
        };
    }

    void test_remove_system_by_name_unknown() {
        "removeSystem(name) returns false and removes nothing for an unknown name"_test = [] {
            ECS::Registry registry;
            ECS::SystemManager systems;
            std::vector<std::string> runs;
            int shutdowns = 0;
            expect(!systems.removeSystem(registry, "Anything"));   // no system at all

            systems.addSystem<NamedSystem>(registry, "Kept", runs, shutdowns);
            expect(!systems.removeSystem(registry, "Unknown"));
            expect(!systems.removeSystem(registry, ""));

            expect(shutdowns == 0);
            expect(systems.size() == std::size_t{1});
        };
    }

    void test_remove_system_by_name_removes_only_the_first() {
        "removeSystem(name) removes only the first of two systems with the same name"_test = [] {
            ECS::Registry registry;
            ECS::SystemManager systems;
            std::vector<std::string> runs;
            int shutdowns = 0;
            systems.addSystem<NamedSystem>(registry, "Twin", runs, shutdowns);
            auto& second = systems.addSystem<NamedSystem>(registry, "Twin", runs, shutdowns);

            expect(systems.removeSystem(registry, "Twin"));

            expect(shutdowns == 1);
            expect(systems.size() == std::size_t{1});
            expect(systems.getSystem("Twin") == &second);
        };
    }

    void test_remove_system_by_name_keeps_the_order() {
        "removeSystem(name) keeps the order of the remaining systems"_test = [] {
            ECS::Registry registry;
            ECS::SystemManager systems;
            std::vector<std::string> runs;
            int shutdowns = 0;
            systems.addSystem<NamedSystem>(registry, "A", runs, shutdowns);
            systems.addSystem<NamedSystem>(registry, "B", runs, shutdowns);
            systems.addSystem<NamedSystem>(registry, "C", runs, shutdowns);
            systems.addSystem<NamedSystem>(registry, "D", runs, shutdowns);

            expect(systems.removeSystem(registry, "B"));
            systems.update(registry);

            expect(runs == std::vector<std::string>{"A", "C", "D"});
        };
    }

    // ---- names

    void test_names_is_empty_without_systems() {
        "names is empty when there is no system"_test = [] {
            const ECS::SystemManager systems;
            expect(systems.names().empty());
        };
    }

    void test_names_lists_systems_in_execution_order() {
        "names lists the systems in execution order, duplicates included"_test = [] {
            ECS::Registry registry;
            ECS::SystemManager systems;
            std::vector<std::string> runs;
            int shutdowns = 0;
            systems.addSystem<NamedSystem>(registry, "Input", runs, shutdowns);
            systems.addSystem(registry, std::make_unique<NamedSystem>("Movement", runs, shutdowns));
            systems.addSystem<NamedSystem>(registry, "Input", runs, shutdowns);

            expect(systems.names() == std::vector<std::string>{"Input", "Movement", "Input"});
        };
    }

    void test_names_follows_removals() {
        "names follows the removal of a system"_test = [] {
            ECS::Registry registry;
            ECS::SystemManager systems;
            std::vector<std::string> runs;
            int shutdowns = 0;
            systems.addSystem<NamedSystem>(registry, "A", runs, shutdowns);
            systems.addSystem<NamedSystem>(registry, "B", runs, shutdowns);
            systems.addSystem<NamedSystem>(registry, "C", runs, shutdowns);

            systems.removeSystem(registry, "B");

            expect(systems.names() == std::vector<std::string>{"A", "C"});
        };
    }

    void test_add_created_system_refuses_null() {
        "addSystem of a null system throws and adds nothing"_test = [] {
            ECS::Registry registry;
            ECS::SystemManager systems;

            expect(throws<std::invalid_argument>([&] { systems.addSystem(registry, std::unique_ptr<ECS::ISystem>{}); }));
            expect(systems.size() == std::size_t{0});
        };
    }
}

void run_system_manager_tests() {
    test_add_system_returns_reference_to_constructed_instance();
    test_size_reflects_registered_systems();
    test_update_runs_systems_in_registration_order();
    test_update_passes_dt_to_systems();
    test_update_runs_a_system_over_a_never_touched_component();
    test_update_moves_entities_matching_the_view();
    test_update_runs_under_deferred_mutation_mode();
    test_update_flushes_between_systems_for_same_frame_visibility();
    test_add_system_calls_init_once();
    test_remove_system_calls_shutdown_once_and_removes_it();
    test_remove_system_returns_false_when_not_found();
    test_add_created_system_takes_it_and_calls_init();
    test_add_created_system_runs_after_existing_ones();
    test_add_created_system_is_found_and_removed_by_type();
    test_add_created_system_refuses_null();
    test_get_system_by_name_finds_it();
    test_get_system_by_name_unknown_or_empty();
    test_get_system_by_name_returns_the_first_of_two();
    test_remove_system_by_name_removes_it();
    test_remove_system_by_name_unknown();
    test_remove_system_by_name_removes_only_the_first();
    test_remove_system_by_name_keeps_the_order();
    test_names_is_empty_without_systems();
    test_names_lists_systems_in_execution_order();
    test_names_follows_removals();
}
