#include <boost/ut.hpp>

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

#include "ECS/Systems.hpp"

using namespace boost::ut;

namespace {
    struct Position { int x; int y; };
    struct Velocity { int x; int y; };

    bool contains(const std::vector<ECS::Entity>& entities, ECS::Entity entity) {
        return std::find(entities.begin(), entities.end(), entity) != entities.end();
    }

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
                _visibleDuringUpdate = contains(reg.getAllEntities(), _spawned);
            }

            const std::string name() const override { return "SpawningSystem"; }

        private:
            ECS::Entity& _spawned;
            bool& _visibleDuringUpdate;
    };

    void test_add_system_returns_reference_to_constructed_instance() {
        "addSystem returns a reference to the constructed instance"_test = [] {
            double lastDt = -1.0;
            ECS::SystemManager systems;
            auto& system = systems.addSystem<DtRecordingSystem>(lastDt);

            expect(system.name() == std::string("DtRecordingSystem"));
        };
    }

    void test_size_reflects_registered_systems() {
        "size reflects the number of registered systems"_test = [] {
            ECS::SystemManager systems;
            expect(systems.size() == std::size_t{0});

            double dtA = 0.0;
            double dtB = 0.0;
            systems.addSystem<DtRecordingSystem>(dtA);
            expect(systems.size() == std::size_t{1});

            systems.addSystem<DtRecordingSystem>(dtB);
            expect(systems.size() == std::size_t{2});
        };
    }

    void test_update_runs_systems_in_registration_order() {
        "update runs systems in registration order"_test = [] {
            ECS::Registry registry;
            ECS::SystemManager systems;
            std::vector<int> order;
            systems.addSystem<OrderRecordingSystem>(order, 1);
            systems.addSystem<OrderRecordingSystem>(order, 2);
            systems.addSystem<OrderRecordingSystem>(order, 3);

            systems.update(registry, 0.0);

            expect(order == std::vector<int>{1, 2, 3});
        };
    }

    void test_update_passes_dt_to_systems() {
        "update passes dt through to each system"_test = [] {
            ECS::Registry registry;
            ECS::SystemManager systems;
            double lastDt = -1.0;
            systems.addSystem<DtRecordingSystem>(lastDt);

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
            systems.addSystem<MovementSystem>();

            systems.update(registry, 1.0);

            expect(!registry.hasComponent<Position>(entity));
        };
    }

    void test_update_moves_entities_matching_the_view() {
        "update moves entities that have both Position and Velocity"_test = [] {
            ECS::Registry registry;
            auto entity = registry.spawnEntity();
            registry.emplaceComponent<Position>(entity, 0, 0);
            registry.emplaceComponent<Velocity>(entity, 2, 3);

            ECS::SystemManager systems;
            systems.addSystem<MovementSystem>();

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
            systems.addSystem<SpawningSystem>(spawned, visibleDuringUpdate);

            systems.update(registry, 0.0);

            expect(!visibleDuringUpdate);
            expect(contains(registry.getAllEntities(), spawned));
        };
    }
}

void run_systems_tests() {
    test_add_system_returns_reference_to_constructed_instance();
    test_size_reflects_registered_systems();
    test_update_runs_systems_in_registration_order();
    test_update_passes_dt_to_systems();
    test_update_runs_a_system_over_a_never_touched_component();
    test_update_moves_entities_matching_the_view();
    test_update_runs_under_deferred_mutation_mode();
}
