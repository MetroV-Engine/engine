#include <boost/ut.hpp>

#include <cstddef>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "ECS/Registry.hpp"

using namespace boost::ut;

namespace {
    struct Position { int x; int y; };
    struct Velocity { int x; int y; };
    struct Health { int value; };
    struct Unregistered { int value; };
    struct BeyondLimit { int value; };
    template<std::size_t N> struct Filler { int value; };

    template<std::size_t... Indices>
    void exhaust_component_ids(std::index_sequence<Indices...>) {
        ((void)ECS::componentId<Filler<Indices>>(), ...);
    }

    void test_registry_is_default_constructible() {
        "registry is default constructible"_test = [] {
            ECS::Registry registry;
            expect(registry.getAllEntities().empty());
        };
    }

    void test_registry_is_not_copyable() {
        "registry is not copyable"_test = [] {
            static_assert(!std::is_copy_constructible_v<ECS::Registry>);
            static_assert(!std::is_copy_assignable_v<ECS::Registry>);
            expect(true);
        };
    }

    void test_spawn_entity_assigns_sequential_indexes() {
        "spawnEntity assigns sequential indexes"_test = [] {
            ECS::Registry registry;
            expect(registry.spawnEntity().index() == std::size_t{0});
            expect(registry.spawnEntity().index() == std::size_t{1});
        };
    }

    void test_spawn_entity_with_name_stores_name() {
        "spawnEntity stores an optional name"_test = [] {
            ECS::Registry registry;
            const auto entity = registry.spawnEntity("Player");
            expect(registry.getEntityName(entity) == "Player");
        };
    }

    void test_spawn_entity_without_name_has_empty_name() {
        "unnamed entities have an empty name"_test = [] {
            ECS::Registry registry;
            const auto entity = registry.spawnEntity();
            expect(registry.getEntityName(entity).empty());
        };
    }

    void test_spawn_entity_reuse_resets_state() {
        "recycled entities do not inherit components or names"_test = [] {
            ECS::Registry registry;
            const auto first = registry.spawnEntity("Old");
            registry.emplaceComponent<Position>(first, 1, 2);
            registry.killEntity(first);
            const auto second = registry.spawnEntity();

            expect(!registry.hasComponent<Position>(second));
            expect(registry.getEntityName(second).empty());
            expect(second.generation() != first.generation());
        };
    }

    void test_entity_from_index_does_not_create_entity() {
        "registry entityFromIndex does not create an entity"_test = [] {
            const ECS::Registry registry;
            const auto entity = registry.entityFromIndex(42);
            expect(entity.index() == std::size_t{42});
            expect(registry.getAllEntities().empty());
        };
    }

    void test_entity_from_index_returns_current_handle() {
        "entityFromIndex returns the current handle"_test = [] {
            ECS::Registry registry;
            const auto created = registry.spawnEntity();
            expect(registry.entityFromIndex(created.index()) == created);
        };
    }

    void test_kill_entity_makes_entity_dead() {
        "killEntity makes the entity dead"_test = [] {
            ECS::Registry registry;
            const auto entity = registry.spawnEntity();
            registry.killEntity(entity);
            expect(registry.getEntityName(entity).empty());
            expect(!registry.hasComponent<Position>(entity));
        };
    }

    void test_kill_entity_rejects_invalid_entity() {
        "killEntity rejects an invalid entity"_test = [] {
            ECS::Registry registry;
            expect(throws<std::invalid_argument>([&] {
                registry.killEntity(ECS::Entity{42});
            }));
        };
    }

    void test_kill_entity_removes_all_components() {
        "killEntity removes all components"_test = [] {
            ECS::Registry registry;
            const auto entity = registry.spawnEntity();
            registry.emplaceComponent<Position>(entity, 1, 2);
            registry.emplaceComponent<Velocity>(entity, 3, 4);
            registry.killEntity(entity);

            expect(!registry.hasComponent<Position>(entity));
            expect(!registry.hasComponent<Velocity>(entity));
        };
    }

    void test_register_component_creates_empty_pool() {
        "registerComponent creates an empty pool"_test = [] {
            ECS::Registry registry;
            auto& storage = registry.registerComponent<Position>();
            expect(storage.size() == std::size_t{0});
            expect(registry.getIf<Position>() == &storage);
        };
    }

    void test_register_component_is_idempotent() {
        "registerComponent is idempotent"_test = [] {
            ECS::Registry registry;
            const auto entity = registry.spawnEntity();
            const auto& first = registry.registerComponent<Position>();
            registry.emplaceComponent<Position>(entity, 1, 2);
            const auto& second = registry.registerComponent<Position>();

            expect(&first == &second);
            expect(second.get(entity.value()).x == 1);
        };
    }

    void test_register_different_components_have_independent_pools() {
        "different component types have independent pools"_test = [] {
            ECS::Registry registry;
            const auto entity = registry.spawnEntity();
            registry.emplaceComponent<Position>(entity, 1, 2);
            registry.emplaceComponent<Velocity>(entity, 3, 4);

            expect(registry.getComponents<Position>().get(entity.value()).x == 1);
            expect(registry.getComponents<Velocity>().get(entity.value()).x == 3);
        };
    }

    void test_get_components_throws_for_unregistered_component() {
        "getComponents throws for an unregistered type"_test = [] {
            ECS::Registry registry;
            expect(throws<std::out_of_range>([&] {
                registry.getComponents<Unregistered>();
            }));
        };
    }

    void test_get_component_returns_stored_component() {
        "getComponent returns the stored component"_test = [] {
            ECS::Registry registry;
            const auto entity = registry.spawnEntity();
            registry.emplaceComponent<Position>(entity, 1, 2);

            expect(registry.getComponent<Position>(entity).x == 1);
            expect(registry.getComponent<Position>(entity).y == 2);
        };
    }

    void test_get_component_returns_mutable_reference() {
        "getComponent returns a mutable reference"_test = [] {
            ECS::Registry registry;
            const auto entity = registry.spawnEntity();
            registry.emplaceComponent<Position>(entity, 1, 2);
            registry.getComponent<Position>(entity).x = 9;

            expect(registry.getComponent<Position>(entity).x == 9);
        };
    }

    void test_get_component_throws_for_missing_component() {
        "getComponent throws for a missing component"_test = [] {
            ECS::Registry registry;
            const auto entity = registry.spawnEntity();

            expect(throws<std::out_of_range>([&] {
                registry.getComponent<Position>(entity);
            }));
        };
    }

    void test_get_component_throws_for_dead_entity() {
        "getComponent rejects a dead entity"_test = [] {
            ECS::Registry registry;
            const auto entity = registry.spawnEntity();
            registry.killEntity(entity);

            expect(throws<std::invalid_argument>([&] {
                registry.getComponent<Position>(entity);
            }));
        };
    }

    void test_has_component_is_false_before_component_is_added() {
        "hasComponent is false before insertion"_test = [] {
            ECS::Registry registry;
            const auto entity = registry.spawnEntity();
            expect(!registry.hasComponent<Position>(entity));
        };
    }

    void test_has_component_reflects_added_component() {
        "hasComponent reflects insertion"_test = [] {
            ECS::Registry registry;
            const auto entity = registry.spawnEntity();
            registry.addComponent(entity, Position{1, 2});

            expect(registry.hasComponent<Position>(entity));
            expect(!registry.hasComponent<Velocity>(entity));
        };
    }

    void test_add_component_replaces_existing_component() {
        "addComponent replaces an existing component"_test = [] {
            ECS::Registry registry;
            const auto entity = registry.spawnEntity();
            registry.addComponent(entity, Position{1, 2});
            auto& stored = registry.addComponent(entity, Position{3, 4});

            expect(stored.x == 3);
            expect(registry.getComponents<Position>().size() == std::size_t{1});
        };
    }

    void test_add_component_rejects_dead_entity() {
        "addComponent rejects a dead entity"_test = [] {
            ECS::Registry registry;
            const auto entity = registry.spawnEntity();
            registry.killEntity(entity);

            expect(throws<std::invalid_argument>([&] {
                registry.addComponent(entity, Position{1, 2});
            }));
        };
    }

    void test_emplace_component_constructs_and_sets_signature() {
        "emplaceComponent constructs and registers a component"_test = [] {
            ECS::Registry registry;
            const auto entity = registry.spawnEntity();
            auto& component = registry.emplaceComponent<Position>(entity, 1, 2);

            expect(component.x == 1);
            expect(registry.hasComponents<Position>(entity));
        };
    }

    void test_emplace_component_replaces_existing_component() {
        "emplaceComponent replaces an existing component"_test = [] {
            ECS::Registry registry;
            const auto entity = registry.spawnEntity();
            registry.emplaceComponent<Position>(entity, 1, 2);
            registry.emplaceComponent<Position>(entity, 3, 4);

            expect(registry.getComponent<Position>(entity).x == 3);
            expect(registry.getComponents<Position>().size() == std::size_t{1});
        };
    }

    void test_remove_component_clears_signature() {
        "removeComponent removes data and clears the signature"_test = [] {
            ECS::Registry registry;
            const auto entity = registry.spawnEntity();
            registry.emplaceComponent<Position>(entity, 1, 2);
            registry.removeComponent<Position>(entity);

            expect(!registry.hasComponent<Position>(entity));
            expect(!registry.hasComponents<Position>(entity));
        };
    }

    void test_remove_unregistered_component_is_noop() {
        "removeComponent is a no-op for an unregistered type"_test = [] {
            ECS::Registry registry;
            const auto entity = registry.spawnEntity();
            registry.removeComponent<Unregistered>(entity);

            expect(!registry.hasComponent<Unregistered>(entity));
        };
    }

    void test_remove_component_does_not_remove_other_components() {
        "removeComponent preserves other components"_test = [] {
            ECS::Registry registry;
            const auto entity = registry.spawnEntity();
            registry.emplaceComponent<Position>(entity, 1, 2);
            registry.emplaceComponent<Velocity>(entity, 3, 4);
            registry.removeComponent<Position>(entity);

            expect(registry.hasComponent<Velocity>(entity));
        };
    }

    void test_has_components_requires_all_requested_types() {
        "hasComponents requires all requested types"_test = [] {
            ECS::Registry registry;
            const auto entity = registry.spawnEntity();
            registry.emplaceComponent<Position>(entity, 1, 2);

            expect(!registry.hasComponents<Position, Velocity>(entity));
            registry.emplaceComponent<Velocity>(entity, 3, 4);
            expect(registry.hasComponents<Position, Velocity>(entity));
        };
    }

    void test_has_components_is_false_for_dead_entity() {
        "hasComponents is false for a dead entity"_test = [] {
            ECS::Registry registry;
            const auto entity = registry.spawnEntity();
            registry.emplaceComponent<Position>(entity, 1, 2);
            registry.killEntity(entity);

            expect(!registry.hasComponents<Position>(entity));
        };
    }

    void test_get_if_returns_null_for_unregistered_component() {
        "getIf returns null for an unregistered type"_test = [] {
            ECS::Registry registry;
            expect(registry.getIf<Unregistered>() == nullptr);
        };
    }

    void test_get_if_returns_registered_storage() {
        "getIf returns a registered storage"_test = [] {
            ECS::Registry registry;
            auto& storage = registry.registerComponent<Position>();

            expect(registry.getIf<Position>() == &storage);
        };
    }

    void test_view_returns_matching_entities() {
        "view returns entities with requested components"_test = [] {
            ECS::Registry registry;
            const auto first = registry.spawnEntity();
            const auto second = registry.spawnEntity();
            registry.emplaceComponent<Position>(first, 1, 2);
            registry.emplaceComponent<Position>(second, 3, 4);

            std::size_t count = 0;
            for (auto [entity, position] : registry.view<Position>()) {
                expect(registry.hasComponent<Position>(entity));
                expect(position.x > 0);
                ++count;
            }
            expect(count == std::size_t{2});
        };
    }

    void test_view_requires_all_requested_components() {
        "view returns only the component intersection"_test = [] {
            ECS::Registry registry;
            const auto first = registry.spawnEntity();
            const auto second = registry.spawnEntity();
            registry.emplaceComponent<Position>(first, 1, 2);
            registry.emplaceComponent<Position>(second, 3, 4);
            registry.emplaceComponent<Velocity>(second, 5, 6);

            std::size_t count = 0;
            for (auto [entity, position, velocity] : registry.view<Position, Velocity>()) {
                expect(entity == second);
                expect(position.x == 3);
                expect(velocity.x == 5);
                ++count;
            }
            expect(count == std::size_t{1});
        };
    }

    void test_view_throws_for_unregistered_component() {
        "view throws for an unregistered component"_test = [] {
            ECS::Registry registry;
            expect(throws<std::out_of_range>([&] {
                registry.view<Unregistered>();
            }));
        };
    }

    void test_const_view_returns_const_components() {
        "const view returns const components"_test = [] {
            ECS::Registry registry;
            const auto entity = registry.spawnEntity();
            registry.emplaceComponent<Position>(entity, 1, 2);
            const ECS::Registry& constRegistry = registry;
            auto query = constRegistry.view<Position>();
            using Value = decltype(*query.begin());

            static_assert(std::is_const_v<
                std::remove_reference_t<std::tuple_element_t<1, Value>>
            >);
            expect(std::get<1>(*query.begin()).x == 1);
        };
    }

    void test_add_system_registers_required_components() {
        "addSystem registers required component types"_test = [] {
            ECS::Registry registry;
            registry.addSystem<Position>([](ECS::Registry&) {});

            expect(registry.getComponents<Position>().size() == std::size_t{0});
        };
    }

    void test_add_system_runs_when_component_pool_is_empty() {
        "addSystem runs with an empty component pool"_test = [] {
            ECS::Registry registry;
            bool ran = false;
            registry.addSystem<Position>([&](ECS::Registry& world) {
                for (auto entry : world.view<Position>()) {
                    (void)entry;
                }
                ran = true;
            });

            registry.runSystems();
            expect(ran);
        };
    }

    void test_run_systems_executes_in_registration_order() {
        "runSystems preserves registration order"_test = [] {
            ECS::Registry registry;
            std::vector<int> order;
            registry.addSystem<Position>([&](ECS::Registry&) { order.push_back(1); });
            registry.addSystem<Velocity>([&](ECS::Registry&) { order.push_back(2); });

            registry.runSystems();
            expect(order == std::vector<int>{1, 2});
        };
    }

    void test_run_systems_executes_again() {
        "runSystems keeps systems registered"_test = [] {
            ECS::Registry registry;
            int runs = 0;
            registry.addSystem<Position>([&](ECS::Registry&) { ++runs; });

            registry.runSystems();
            registry.runSystems();
            expect(runs == 2);
        };
    }

    void test_get_all_entities_is_empty_for_new_registry() {
        "getAllEntities is empty for a new registry"_test = [] {
            const ECS::Registry registry;
            expect(registry.getAllEntities().empty());
        };
    }

    void test_get_all_entities_reuses_cache_without_changes() {
        "getAllEntities reuses its cache without changes"_test = [] {
            ECS::Registry registry;
            registry.spawnEntity();
            const auto& first = registry.getAllEntities();
            const auto& second = registry.getAllEntities();

            expect(&first == &second);
        };
    }

    void test_get_all_entities_reflects_spawns_and_kills() {
        "getAllEntities reflects spawns and kills"_test = [] {
            ECS::Registry registry;
            const auto first = registry.spawnEntity();
            const auto second = registry.spawnEntity();
            registry.killEntity(first);

            expect(registry.getAllEntities().size() == std::size_t{1});
            expect(registry.getAllEntities()[0] == second);
            registry.spawnEntity();
            expect(registry.getAllEntities().size() == std::size_t{2});
        };
    }

    void test_get_all_entities_preserves_current_generations() {
        "getAllEntities preserves current generations"_test = [] {
            ECS::Registry registry;
            const auto oldEntity = registry.spawnEntity();
            registry.killEntity(oldEntity);
            const auto newEntity = registry.spawnEntity();

            expect(registry.getAllEntities()[0] == newEntity);
            expect(registry.getAllEntities()[0].generation() != oldEntity.generation());
        };
    }

    void test_set_entity_name_assigns_and_replaces_name() {
        "setEntityName assigns and replaces a name"_test = [] {
            ECS::Registry registry;
            const auto entity = registry.spawnEntity();
            registry.setEntityName(entity, "First");
            registry.setEntityName(entity, "Second");

            expect(registry.getEntityName(entity) == "Second");
        };
    }

    void test_set_entity_name_rejects_dead_entity() {
        "setEntityName rejects a dead entity"_test = [] {
            ECS::Registry registry;
            const auto entity = registry.spawnEntity();
            registry.killEntity(entity);

            expect(throws<std::invalid_argument>([&] {
                registry.setEntityName(entity, "Dead");
            }));
        };
    }

    void test_set_entity_name_invalidates_cache() {
        "setEntityName invalidates the entity cache"_test = [] {
            ECS::Registry registry;
            const auto entity = registry.spawnEntity();
            const auto& first = registry.getAllEntities();
            registry.setEntityName(entity, "Named");
            const auto& second = registry.getAllEntities();

            expect(&first == &second);
            expect(registry.getEntityName(entity) == "Named");
        };
    }

    void test_get_entity_name_returns_empty_for_unknown_or_dead_entity() {
        "getEntityName is empty for unknown or dead entities"_test = [] {
            ECS::Registry registry;
            const auto entity = registry.spawnEntity("Player");
            registry.killEntity(entity);

            expect(registry.getEntityName(entity).empty());
            expect(registry.getEntityName(ECS::Entity{42}).empty());
        };
    }

    void test_registry_hands_out_read_only_storages() {
        "registry only hands out read-only storages"_test = [] {
            ECS::Registry registry;
            using Storage = ECS::ComponentStorage<Position>;
            static_assert(std::is_same_v<
                decltype(registry.registerComponent<Position>()), const Storage&>);
            static_assert(std::is_same_v<
                decltype(registry.getComponents<Position>()), const Storage&>);
            static_assert(std::is_same_v<
                decltype(registry.getIf<Position>()), const Storage*>);
            expect(true);
        };
    }

    void test_signature_mirrors_the_pools() {
        "hasComponent and hasComponents agree through every change"_test = [] {
            ECS::Registry registry;
            const auto agree = [&](ECS::Entity entity) {
                return registry.hasComponent<Position>(entity)
                        == registry.hasComponents<Position>(entity)
                    && registry.hasComponent<Velocity>(entity)
                        == registry.hasComponents<Velocity>(entity);
            };

            const auto entity = registry.spawnEntity();
            expect(agree(entity));

            registry.emplaceComponent<Position>(entity, 1, 2);
            expect(agree(entity));
            expect(registry.hasComponents<Position>(entity));

            registry.addComponent(entity, Position{3, 4});
            registry.emplaceComponent<Velocity>(entity, 5, 6);
            expect(agree(entity));
            expect(registry.hasComponents<Position, Velocity>(entity));

            registry.removeComponent<Position>(entity);
            expect(agree(entity));
            expect(!registry.hasComponents<Position>(entity));
            expect(registry.hasComponents<Velocity>(entity));

            registry.killEntity(entity);
            expect(agree(entity));

            const auto recycled = registry.spawnEntity();
            expect(recycled.index() == entity.index());
            expect(agree(recycled));
            expect(!registry.hasComponent<Velocity>(recycled));
            expect(!registry.hasComponents<Velocity>(recycled));
        };
    }

    // Must stay the last test run: componentId<T>() is process-wide, so this
    // consumes every remaining ID and any type first used afterwards would be
    // past the Signature limit.
    void test_component_types_beyond_signature_limit_are_rejected() {
        "component types beyond the signature limit are rejected before insertion"_test = [] {
            exhaust_component_ids(std::make_index_sequence<ECS::MaxComponentTypes>{});

            ECS::Registry registry;
            const auto entity = registry.spawnEntity();

            expect(throws<std::out_of_range>([&] {
                registry.registerComponent<BeyondLimit>();
            }));
            expect(throws<std::out_of_range>([&] {
                registry.addSystem<BeyondLimit>([](ECS::Registry&) {});
            }));
            expect(throws<std::out_of_range>([&] {
                registry.addComponent(entity, BeyondLimit{1});
            }));
            expect(throws<std::out_of_range>([&] {
                registry.emplaceComponent<BeyondLimit>(entity, 1);
            }));

            expect(registry.getIf<BeyondLimit>() == nullptr);
            expect(!registry.hasComponent<BeyondLimit>(entity));
            expect(!registry.hasComponents<BeyondLimit>(entity));
            registry.removeComponent<BeyondLimit>(entity);
            expect(!registry.hasComponent<BeyondLimit>(entity));
        };
    }
}

void run_registry_tests() {
    test_registry_is_default_constructible();
    test_registry_is_not_copyable();
    test_spawn_entity_assigns_sequential_indexes();
    test_spawn_entity_with_name_stores_name();
    test_spawn_entity_without_name_has_empty_name();
    test_spawn_entity_reuse_resets_state();
    test_entity_from_index_does_not_create_entity();
    test_entity_from_index_returns_current_handle();
    test_kill_entity_makes_entity_dead();
    test_kill_entity_rejects_invalid_entity();
    test_kill_entity_removes_all_components();
    test_register_component_creates_empty_pool();
    test_register_component_is_idempotent();
    test_register_different_components_have_independent_pools();
    test_get_components_throws_for_unregistered_component();
    test_get_component_returns_stored_component();
    test_get_component_returns_mutable_reference();
    test_get_component_throws_for_missing_component();
    test_get_component_throws_for_dead_entity();
    test_has_component_is_false_before_component_is_added();
    test_has_component_reflects_added_component();
    test_add_component_replaces_existing_component();
    test_add_component_rejects_dead_entity();
    test_emplace_component_constructs_and_sets_signature();
    test_emplace_component_replaces_existing_component();
    test_remove_component_clears_signature();
    test_remove_unregistered_component_is_noop();
    test_remove_component_does_not_remove_other_components();
    test_has_components_requires_all_requested_types();
    test_has_components_is_false_for_dead_entity();
    test_get_if_returns_null_for_unregistered_component();
    test_get_if_returns_registered_storage();
    test_registry_hands_out_read_only_storages();
    test_signature_mirrors_the_pools();
    test_view_returns_matching_entities();
    test_view_requires_all_requested_components();
    test_view_throws_for_unregistered_component();
    test_const_view_returns_const_components();
    test_add_system_registers_required_components();
    test_add_system_runs_when_component_pool_is_empty();
    test_run_systems_executes_in_registration_order();
    test_run_systems_executes_again();
    test_get_all_entities_is_empty_for_new_registry();
    test_get_all_entities_reuses_cache_without_changes();
    test_get_all_entities_reflects_spawns_and_kills();
    test_get_all_entities_preserves_current_generations();
    test_set_entity_name_assigns_and_replaces_name();
    test_set_entity_name_rejects_dead_entity();
    test_set_entity_name_invalidates_cache();
    test_get_entity_name_returns_empty_for_unknown_or_dead_entity();
    test_component_types_beyond_signature_limit_are_rejected();  // keep last
}
