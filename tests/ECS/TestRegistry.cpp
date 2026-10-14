#include <boost/ut.hpp>

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "ECS/Core/Component/ComponentPool.hpp"
#include "ECS/Registry.hpp"
#include "RegistryTestAccess.hpp"

using namespace boost::ut;

namespace {
    struct Position { int x; int y; };
    struct Velocity { int x; int y; };
    struct Health { int value; };
    struct Clock { double elapsed{0.0}; };
    struct Settings { std::string name; };
    struct Unregistered { int value; };
    struct BeyondLimit { int value; };


    // Types and helpers of the ComponentId-based core tests.
    using Ids = ECS::TestAccess::RegistryIdAccess;

    struct IdPosition { int x = 7; int y = 8; };
    struct IdVelocity { int dx = 0; };
    struct IdNoDefault {
        explicit IdNoDefault(int v) : value(v) {}
        int value;
    };

    template<typename T>
    ECS::ComponentId idOf() { return ECS::componentId<T>(); }
    template<std::size_t N> struct Filler { int value; };

    template<std::size_t... Indices>
    void exhaust_component_ids(std::index_sequence<Indices...>) {
        ((void)ECS::componentId<Filler<Indices>>(), ...);
    }

    void test_registry_is_default_constructible() {
        "registry is default constructible"_test = [] {
            ECS::Registry registry;
            expect(registry.entityCount() == std::size_t{0});
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

            expect(!registry.hasComponents<Position>(second));
            expect(registry.getEntityName(second).empty());
            expect(second.generation() != first.generation());
        };
    }

    void test_spawn_entity_stamps_registry_world() {
        "spawnEntity stamps the registry's world id on the handle"_test = [] {
            ECS::Registry registry;
            expect(registry.worldId() != ECS::InvalidWorld);
            expect(registry.spawnEntity().world() == registry.worldId());
        };
    }

    void test_registries_have_distinct_world_ids() {
        "each registry has its own world id"_test = [] {
            const ECS::Registry first;
            const ECS::Registry second;
            expect(first.worldId() != second.worldId());
        };
    }

    void test_registry_rejects_entity_from_another_registry() {
        "a registry rejects a handle issued by another registry"_test = [] {
            ECS::Registry worldA;
            ECS::Registry worldB;
            const auto a = worldA.spawnEntity("A");
            const auto b = worldB.spawnEntity("B");
            worldA.emplaceComponent<Position>(a, 10, 20);
            worldB.emplaceComponent<Position>(b, 999, 999);

            // Same index and generation: only the world id tells them apart.
            expect(a.index() == b.index());
            expect(a.generation() == b.generation());

            expect(throws<std::invalid_argument>([&] {
                (void)worldB.getComponent<Position>(a);
            }));
            expect(throws<std::invalid_argument>([&] {
                worldB.emplaceComponent<Velocity>(a, 1, 1);
            }));
            expect(throws<std::invalid_argument>([&] {
                worldB.killEntity(a);
            }));
            expect(!worldB.hasComponents<Position>(a));
            expect(worldB.getEntityName(a).empty());

            expect(worldB.getComponent<Position>(b).x == 999);
            expect(worldA.getComponent<Position>(a).x == 10);
        };
    }

    void test_kill_entity_makes_entity_dead() {
        "killEntity makes the entity dead"_test = [] {
            ECS::Registry registry;
            const auto entity = registry.spawnEntity();
            registry.killEntity(entity);
            expect(registry.getEntityName(entity).empty());
            expect(!registry.hasComponents<Position>(entity));
        };
    }

    void test_kill_entity_rejects_invalid_entity() {
        "killEntity rejects an invalid entity"_test = [] {
            ECS::Registry registry;
            expect(throws<std::invalid_argument>([&] {
                registry.killEntity(ECS::Entity{});
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

            expect(!registry.hasComponents<Position>(entity));
            expect(!registry.hasComponents<Velocity>(entity));
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
        "hasComponents is false before insertion"_test = [] {
            ECS::Registry registry;
            const auto entity = registry.spawnEntity();
            expect(!registry.hasComponents<Position>(entity));
        };
    }

    void test_has_component_reflects_added_component() {
        "hasComponents reflects insertion"_test = [] {
            ECS::Registry registry;
            const auto entity = registry.spawnEntity();
            registry.addComponent(entity, Position{1, 2});

            expect(registry.hasComponents<Position>(entity));
            expect(!registry.hasComponents<Velocity>(entity));
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

            expect(!registry.hasComponents<Position>(entity));
            expect(!registry.hasComponents<Position>(entity));
        };
    }

    void test_remove_unregistered_component_is_noop() {
        "removeComponent is a no-op for an unregistered type"_test = [] {
            ECS::Registry registry;
            const auto entity = registry.spawnEntity();
            registry.removeComponent<Unregistered>(entity);

            expect(!registry.hasComponents<Unregistered>(entity));
        };
    }

    void test_remove_component_does_not_remove_other_components() {
        "removeComponent preserves other components"_test = [] {
            ECS::Registry registry;
            const auto entity = registry.spawnEntity();
            registry.emplaceComponent<Position>(entity, 1, 2);
            registry.emplaceComponent<Velocity>(entity, 3, 4);
            registry.removeComponent<Position>(entity);

            expect(registry.hasComponents<Velocity>(entity));
        };
    }

    void test_on_remove_fires_via_remove_component() {
        "onRemove fires when removeComponent erases the instance"_test = [] {
            ECS::Registry registry;
            const auto entity = registry.spawnEntity();
            registry.emplaceComponent<Position>(entity, 5, 6);
            int callCount = 0;
            Position seen{0, 0};
            registry.onRemove<Position>([&](Position& position) {
                ++callCount;
                seen = position;
            });

            registry.removeComponent<Position>(entity);

            expect(callCount == 1);
            expect(seen.x == 5);
            expect(seen.y == 6);
        };
    }

    void test_on_remove_fires_via_kill_entity() {
        "onRemove fires when killEntity erases the instance"_test = [] {
            ECS::Registry registry;
            const auto entity = registry.spawnEntity();
            registry.emplaceComponent<Health>(entity, 42);
            int callCount = 0;
            registry.onRemove<Health>([&callCount](Health&) { ++callCount; });

            registry.killEntity(entity);

            expect(callCount == 1);
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
                expect(registry.hasComponents<Position>(entity));
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

    void test_view_registers_unregistered_component_and_returns_no_matches() {
        "view registers an unregistered component and returns no matches"_test = [] {
            ECS::Registry registry;
            std::size_t count = 0;
            for (auto entry : registry.view<Unregistered>()) {
                (void)entry;
                ++count;
            }
            expect(count == std::size_t{0});
            expect(registry.getComponents<Unregistered>().size() == std::size_t{0});
        };
    }

    void test_const_view_is_empty_for_unregistered_component() {
        "const view of an unregistered component is empty and does not register it"_test = [] {
            ECS::Registry registry;
            const auto entity = registry.spawnEntity();
            registry.emplaceComponent<Position>(entity, 1, 2);
            const ECS::Registry& constRegistry = registry;

            std::size_t matches = 0;
            for ([[maybe_unused]] auto row : constRegistry.view<Position, Unregistered>()) {
                ++matches;
            }

            expect(matches == std::size_t{0});
            expect(constRegistry.getIf<Unregistered>() == nullptr);
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

    void test_is_alive_is_true_for_a_spawned_entity() {
        "isAlive is true for a spawned entity"_test = [] {
            ECS::Registry registry;
            const auto entity = registry.spawnEntity();

            expect(registry.isAlive(entity));
        };
    }

    void test_is_alive_is_false_after_kill() {
        "isAlive is false after the entity is killed"_test = [] {
            ECS::Registry registry;
            const auto entity = registry.spawnEntity();
            registry.killEntity(entity);

            expect(!registry.isAlive(entity));
        };
    }

    void test_is_alive_rejects_foreign_default_and_stale_handles() {
        "isAlive is false for a default, foreign or recycled handle"_test = [] {
            ECS::Registry registry;
            ECS::Registry other;
            const auto old = registry.spawnEntity();
            registry.killEntity(old);
            const auto recycled = registry.spawnEntity();
            const auto foreign = other.spawnEntity();

            expect(recycled.index() == old.index());
            expect(!registry.isAlive(old));
            expect(registry.isAlive(recycled));
            expect(!registry.isAlive(foreign));
            expect(!registry.isAlive(ECS::Entity{}));
        };
    }

    void test_is_alive_during_deferred_spawn_and_kill() {
        "isAlive follows deferred spawns and kills at flush"_test = [] {
            ECS::Registry registry;
            const auto killed = registry.spawnEntity();
            ECS::Entity spawned{};
            bool spawnedAliveDuring = true;
            bool killedAliveDuring = false;

            registry.runDeferred([&] {
                spawned = registry.spawnEntity();
                registry.killEntity(killed);
                spawnedAliveDuring = registry.isAlive(spawned);
                killedAliveDuring = registry.isAlive(killed);
            });

            expect(!spawnedAliveDuring);
            expect(killedAliveDuring);
            expect(registry.isAlive(spawned));
            expect(!registry.isAlive(killed));
        };
    }

    void test_entity_count_is_zero_for_a_new_registry() {
        "entityCount is zero for a new registry"_test = [] {
            const ECS::Registry registry;
            expect(registry.entityCount() == std::size_t{0});
        };
    }

    void test_entity_count_follows_spawns_and_kills() {
        "entityCount follows spawns and kills"_test = [] {
            ECS::Registry registry;
            const auto first = registry.spawnEntity();
            registry.spawnEntity();
            registry.killEntity(first);
            registry.spawnEntity();

            expect(registry.entityCount() == std::size_t{2});
        };
    }

    void test_entity_count_ignores_queued_spawns() {
        "entityCount does not count spawns queued under runDeferred"_test = [] {
            ECS::Registry registry;
            registry.spawnEntity();
            std::size_t countDuring = 0;

            registry.runDeferred([&] {
                registry.spawnEntity();
                countDuring = registry.entityCount();
            });

            expect(countDuring == std::size_t{1});
            expect(registry.entityCount() == std::size_t{2});
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

    void test_get_entity_name_returns_empty_for_unknown_or_dead_entity() {
        "getEntityName is empty for unknown or dead entities"_test = [] {
            ECS::Registry registry;
            const auto entity = registry.spawnEntity("Player");
            registry.killEntity(entity);

            expect(registry.getEntityName(entity).empty());
            expect(registry.getEntityName(ECS::Entity{}).empty());
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
                return registry.hasComponents<Position>(entity)
                        == registry.hasComponents<Position>(entity)
                    && registry.hasComponents<Velocity>(entity)
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
            expect(!registry.hasComponents<Velocity>(recycled));
            expect(!registry.hasComponents<Velocity>(recycled));
        };
    }

    // componentId<T>() is process-wide. Each test below first uses up 256
    // ids with Filler types (a no-op once done), which guarantees BeyondLimit
    // has an id of 256 or more: past what the old fixed-size signature held.
    void push_component_ids_past_256() {
        exhaust_component_ids(std::make_index_sequence<256>{});
    }

    void test_more_than_256_component_types_can_be_used() {
        "more than 256 component types can be registered and used"_test = [] {
            push_component_ids_past_256();
            expect(ECS::componentId<BeyondLimit>() >= ECS::ComponentId{256});

            ECS::Registry registry;
            const auto entity = registry.spawnEntity();

            expect(nothrow([&] { registry.registerComponent<BeyondLimit>(); }));
            expect(registry.getIf<BeyondLimit>() != nullptr);
            expect(!registry.hasComponents<BeyondLimit>(entity));

            registry.addComponent(entity, BeyondLimit{7});
            expect(registry.hasComponents<BeyondLimit>(entity));
            expect(registry.getComponent<BeyondLimit>(entity).value == 7);

            registry.removeComponent<BeyondLimit>(entity);
            expect(!registry.hasComponents<BeyondLimit>(entity));
        };
    }

    void test_growing_signatures_keeps_existing_components() {
        "registering a high-id type keeps every entity's components"_test = [] {
            push_component_ids_past_256();
            // The registry must start narrower than BeyondLimit needs.
            expect(ECS::componentId<Position>() < ECS::ComponentId{64});
            expect(ECS::componentId<Velocity>() < ECS::ComponentId{64});

            ECS::Registry registry;
            const auto first = registry.spawnEntity();
            const auto second = registry.spawnEntity();
            registry.emplaceComponent<Position>(first, 1, 2);
            registry.emplaceComponent<Velocity>(second, 3, 4);
            const Position* before = &registry.getComponent<Position>(first);

            // First use of BeyondLimit in this registry: signatures widen here.
            registry.emplaceComponent<BeyondLimit>(second, 9);

            expect(registry.hasComponents<Position>(first));
            expect(!registry.hasComponents<Velocity>(first));
            expect(!registry.hasComponents<BeyondLimit>(first));
            expect(registry.hasComponents<Velocity, BeyondLimit>(second));
            expect(!registry.hasComponents<Position>(second));

            expect(&registry.getComponent<Position>(first) == before);
            expect(before->x == 1);
            expect(before->y == 2);
            expect(registry.getComponent<BeyondLimit>(second).value == 9);
        };
    }

    void test_recycled_entity_owns_no_high_id_component() {
        "a recycled entity owns no high-id component"_test = [] {
            push_component_ids_past_256();

            ECS::Registry registry;
            const auto entity = registry.spawnEntity();
            registry.emplaceComponent<Position>(entity, 1, 2);
            registry.emplaceComponent<BeyondLimit>(entity, 3);
            registry.killEntity(entity);

            const auto recycled = registry.spawnEntity();
            expect(recycled.index() == entity.index());
            expect(!registry.hasComponents<Position>(recycled));
            expect(!registry.hasComponents<BeyondLimit>(recycled));
        };
    }

    void test_high_id_component_added_during_deferred_update() {
        "a high-id component added during a deferred update is applied"_test = [] {
            push_component_ids_past_256();

            ECS::Registry registry;
            const auto entity = registry.spawnEntity();
            registry.emplaceComponent<Position>(entity, 1, 2);

            registry.runDeferred([&] {
                registry.emplaceComponent<BeyondLimit>(entity, 5);
                expect(!registry.hasComponents<BeyondLimit>(entity));
                expect(registry.hasComponents<Position>(entity));
            });

            expect(registry.hasComponents<Position, BeyondLimit>(entity));
            expect(registry.getComponent<BeyondLimit>(entity).value == 5);
        };
    }

    void test_type_registered_in_another_registry_is_absent() {
        "a type registered in another registry is simply absent"_test = [] {
            push_component_ids_past_256();
            // narrow must stay narrower than BeyondLimit needs.
            expect(ECS::componentId<Position>() < ECS::ComponentId{64});

            ECS::Registry wide;
            ECS::Registry narrow;
            wide.registerComponent<BeyondLimit>();

            const auto entity = narrow.spawnEntity();
            narrow.emplaceComponent<Position>(entity, 1, 2);

            expect(narrow.getIf<BeyondLimit>() == nullptr);
            expect(!narrow.hasComponents<BeyondLimit>(entity));
            expect(!narrow.hasComponents<Position, BeyondLimit>(entity));
            narrow.removeComponent<BeyondLimit>(entity);
            expect(narrow.hasComponents<Position>(entity));
        };
    }

    void test_set_resource_returns_stored_reference() {
        "setResource returns a reference to the stored value"_test = [] {
            ECS::Registry registry;
            Clock& clock = registry.setResource<Clock>(Clock{1.5});

            expect(clock.elapsed == 1.5_d);
        };
    }

    void test_has_resource_is_false_before_set() {
        "hasResource is false before the resource is set"_test = [] {
            const ECS::Registry registry;

            expect(!registry.hasResource<Clock>());
        };
    }

    void test_has_resource_is_true_after_set() {
        "hasResource is true after the resource is set"_test = [] {
            ECS::Registry registry;
            registry.setResource<Clock>();

            expect(registry.hasResource<Clock>());
        };
    }

    void test_get_resource_returns_the_set_value() {
        "getResource returns the previously set value"_test = [] {
            ECS::Registry registry;
            registry.setResource<Settings>(Settings{"Metro"});

            expect(registry.getResource<Settings>().name == "Metro");
        };
    }

    void test_get_resource_on_unset_type_throws() {
        "getResource on an unset type throws"_test = [] {
            ECS::Registry registry;

            expect(throws<std::out_of_range>([&] { (void)registry.getResource<Clock>(); }));
        };
    }

    void test_set_resource_twice_overwrites_previous_value() {
        "setResource overwrites a previously set value"_test = [] {
            ECS::Registry registry;
            registry.setResource<Clock>(Clock{1.0});
            registry.setResource<Clock>(Clock{2.0});

            expect(registry.getResource<Clock>().elapsed == 2.0_d);
        };
    }

    void test_resources_are_independent_per_type() {
        "different resource types are stored independently"_test = [] {
            ECS::Registry registry;
            registry.setResource<Clock>(Clock{3.0});
            registry.setResource<Settings>(Settings{"World"});

            expect(registry.getResource<Clock>().elapsed == 3.0_d);
            expect(registry.getResource<Settings>().name == "World");
        };
    }

    void test_resource_and_component_ids_do_not_collide() {
        "a type can be both a component and a resource without colliding"_test = [] {
            ECS::Registry registry;
            registry.setResource<Position>(Position{1, 2});
            const auto entity = registry.spawnEntity();
            registry.emplaceComponent<Position>(entity, 10, 20);

            expect(registry.getResource<Position>().x == 1);
            expect(registry.getComponent<Position>(entity).x == 10);
        };
    }

    void test_get_resource_returns_mutable_reference() {
        "getResource returns a mutable reference"_test = [] {
            ECS::Registry registry;
            registry.setResource<Clock>(Clock{0.0});

            registry.getResource<Clock>().elapsed = 42.0;

            expect(registry.getResource<Clock>().elapsed == 42.0_d);
        };
    }

    void test_get_resource_is_const_accessible() {
        "getResource is available on a const registry"_test = [] {
            ECS::Registry registry;
            registry.setResource<Clock>(Clock{5.0});
            const ECS::Registry& constRegistry = registry;

            expect(constRegistry.getResource<Clock>().elapsed == 5.0_d);
        };
    }

    void test_const_get_resource_on_unset_type_throws() {
        "getResource on a const registry throws for an unset type"_test = [] {
            const ECS::Registry registry;

            expect(throws<std::out_of_range>([&] { (void)registry.getResource<Settings>(); }));
        };
    }

    // ------------------------------------------------------------------------
    // Deferred mode (runDeferred)
    // ------------------------------------------------------------------------

    // --- Group A: visibility timing while runDeferred() is active ---

    void test_spawn_during_run_deferred_is_not_visible_until_flush() {
        "spawnEntity during runDeferred is not visible until flush"_test = [] {
            ECS::Registry registry;
            ECS::Entity spawned{};
            bool visibleDuringDefer = true;

            registry.runDeferred([&] {
                spawned = registry.spawnEntity();
                visibleDuringDefer = registry.isAlive(spawned);
            });

            expect(!visibleDuringDefer);
            expect(registry.isAlive(spawned));
        };
    }

    void test_add_component_during_run_deferred_is_not_visible_until_flush() {
        "addComponent during runDeferred is not visible until flush"_test = [] {
            ECS::Registry registry;
            auto entity = registry.spawnEntity();
            bool hasDuringDefer = false;

            registry.runDeferred([&] {
                registry.addComponent(entity, Position{1, 2});
                hasDuringDefer = registry.hasComponents<Position>(entity);
            });

            expect(!hasDuringDefer);
            expect(registry.hasComponents<Position>(entity));
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
                hasDuringDefer = registry.hasComponents<Position>(entity);
            });

            expect(hasDuringDefer);
            expect(!registry.hasComponents<Position>(entity));
        };
    }

    void test_kill_during_run_deferred_is_not_visible_until_flush() {
        "killEntity during runDeferred is not visible until flush"_test = [] {
            ECS::Registry registry;
            auto entity = registry.spawnEntity();
            bool aliveDuringDefer = false;

            registry.runDeferred([&] {
                registry.killEntity(entity);
                aliveDuringDefer = registry.isAlive(entity);
            });

            expect(aliveDuringDefer);
            expect(!registry.isAlive(entity));
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
                secondStepSawIt = registry.hasComponents<Position>(spawned);
            });

            expect(!secondStepSawIt);
            expect(registry.hasComponents<Position>(spawned));
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

            expect(registry.hasComponents<Position>(spawned));
            expect(registry.getComponent<Position>(spawned).x == 7);
            expect(registry.getComponent<Position>(spawned).y == 9);
        };
    }

    void test_mutations_outside_run_deferred_remain_immediate() {
        "mutations outside runDeferred remain immediate"_test = [] {
            ECS::Registry registry;
            auto entity = registry.spawnEntity();
            expect(registry.isAlive(entity));

            registry.addComponent(entity, Position{3, 4});
            expect(registry.hasComponents<Position>(entity));

            registry.removeComponent<Position>(entity);
            expect(!registry.hasComponents<Position>(entity));

            registry.killEntity(entity);
            expect(!registry.isAlive(entity));
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

            expect(registry.entityCount() == std::size_t{1});
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

            expect(registry.isAlive(spawned));

            auto entity = registry.spawnEntity();
            registry.addComponent(entity, Position{5, 6});
            expect(registry.hasComponents<Position>(entity));
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
            expect(registry.hasComponents<Position>(extra));
        };
    }
    // --- Group C: dead entities inside a deferred batch are no-ops ---

    void test_double_kill_in_same_deferred_batch_does_not_throw() {
        "killing the same entity twice in one deferred batch does not throw"_test = [] {
            ECS::Registry registry;
            auto entity = registry.spawnEntity();

            registry.runDeferred([&] {
                registry.killEntity(entity);
                registry.killEntity(entity);
            });

            expect(!registry.isAlive(entity));
        };
    }

    void test_add_component_after_kill_in_same_deferred_batch_is_a_no_op() {
        "addComponent after killEntity on the same entity in one deferred batch is a no-op"_test = [] {
            ECS::Registry registry;
            auto entity = registry.spawnEntity();

            registry.runDeferred([&] {
                registry.killEntity(entity);
                registry.addComponent(entity, Position{1, 2});
            });

            auto reused = registry.spawnEntity();
            expect(!registry.hasComponents<Position>(reused));
        };
    }

    void test_emplace_component_after_kill_in_same_deferred_batch_is_a_no_op() {
        "emplaceComponent after killEntity on the same entity in one deferred batch is a no-op"_test = [] {
            ECS::Registry registry;
            auto entity = registry.spawnEntity();

            registry.runDeferred([&] {
                registry.killEntity(entity);
                registry.emplaceComponent<Position>(entity, 1, 2);
            });

            auto reused = registry.spawnEntity();
            expect(!registry.hasComponents<Position>(reused));
        };
    }

    void test_remove_component_after_kill_in_same_deferred_batch_is_a_no_op() {
        "removeComponent after killEntity on the same entity in one deferred batch does not throw"_test = [] {
            ECS::Registry registry;
            auto entity = registry.spawnEntity();
            registry.addComponent(entity, Position{1, 2});

            registry.runDeferred([&] {
                registry.killEntity(entity);
                registry.removeComponent<Position>(entity);
            });

            expect(!registry.isAlive(entity));
        };
    }
    void test_nested_run_deferred_applies_at_the_end_of_the_outer_call() {
        "a nested runDeferred applies its mutations when the outer call ends"_test = [] {
            ECS::Registry registry;
            auto entity = registry.spawnEntity();
            bool visibleAfterInner = true;

            registry.runDeferred([&] {
                registry.runDeferred([&] {
                    registry.addComponent(entity, Position{1, 2});
                });
                visibleAfterInner = registry.hasComponents<Position>(entity);
            });

            expect(!visibleAfterInner);
            expect(registry.hasComponents<Position>(entity));
        };
    }

    // ------------------------------------------------------------------------
    // ComponentId-based core: the private, type-erased access used by
    // reflect::RegistryInspector (a friend), reached here through
    // RegistryTestAccess. The typed has/remove API forwards to it.
    // ------------------------------------------------------------------------

    void test_ensure_pool_creates_once() {
        "ensurePoolById creates the pool once and keeps an existing one"_test = [] {
            ECS::Registry registry;
            const auto entity = registry.spawnEntity();

            expect(!Ids::hasPoolById(registry, idOf<IdPosition>()));
            Ids::ensurePoolById(registry, idOf<IdPosition>(), ECS::ComponentOps::of<IdPosition>().makePool);
            expect(Ids::hasPoolById(registry, idOf<IdPosition>()));

            registry.addComponent(entity, IdPosition{1, 2});
            Ids::ensurePoolById(registry, idOf<IdPosition>(), ECS::ComponentOps::of<IdPosition>().makePool);
            expect(registry.getComponent<IdPosition>(entity).x == 1_i);
        };
    }

    void test_has_components_by_id() {
        "hasComponentsById agrees with the typed overload"_test = [] {
            ECS::Registry registry;
            const auto entity = registry.spawnEntity();
            registry.addComponent(entity, IdPosition{});

            expect(Ids::hasComponentsById(registry, entity, {idOf<IdPosition>()}));
            expect(!Ids::hasComponentsById(registry, entity, {idOf<IdPosition>(), idOf<IdVelocity>()}));
            expect(Ids::hasComponentsById(registry, entity, {}));
            expect(!Ids::hasComponentsById(registry, entity, {ECS::ComponentId{100000}}));   // never registered

            registry.killEntity(entity);
            expect(!Ids::hasComponentsById(registry, entity, {idOf<IdPosition>()}));
        };
    }

    void test_get_component_by_id() {
        "getComponentById returns the component address or nullptr"_test = [] {
            ECS::Registry registry;
            const auto entity = registry.spawnEntity();
            const auto other = registry.spawnEntity();
            registry.addComponent(entity, IdPosition{3, 4});

            expect(Ids::getComponentById<IdPosition>(registry, entity) == &registry.getComponent<IdPosition>(entity));
            expect(Ids::getComponentById<IdPosition>(registry, other) == nullptr);   // no component
            expect(Ids::getComponentById<IdVelocity>(registry, entity) == nullptr);  // no pool

            registry.killEntity(entity);
            expect(throws<std::invalid_argument>([&] { (void)Ids::getComponentById<IdPosition>(registry, entity); }));
        };
    }

    void test_add_default_component_immediate() {
        "addDefaultComponentById constructs a default component and updates the signature row"_test = [] {
            ECS::Registry registry;
            const auto entity = registry.spawnEntity();
            registry.registerComponent<IdPosition>();

            void* added = Ids::addDefaultComponentById<IdPosition>(registry, entity);

            expect(registry.hasComponents<IdPosition>(entity));
            expect(added == &registry.getComponent<IdPosition>(entity));
            expect(registry.getComponent<IdPosition>(entity).x == 7_i);
        };
    }

    void test_add_default_component_replaces() {
        "addDefaultComponentById replaces an existing component with a default one"_test = [] {
            ECS::Registry registry;
            const auto entity = registry.spawnEntity();
            registry.addComponent(entity, IdPosition{1, 1});

            (void)Ids::addDefaultComponentById<IdPosition>(registry, entity);

            expect(registry.getComponent<IdPosition>(entity).x == 7_i);
            expect(registry.getComponents<IdPosition>().size() == 1_ul);
        };
    }

    void test_add_default_component_errors() {
        "addDefaultComponentById needs a live entity, a pool and a default constructor"_test = [] {
            ECS::Registry registry;
            const auto entity = registry.spawnEntity();
            registry.registerComponent<IdNoDefault>();

            expect(throws<std::out_of_range>([&] { (void)Ids::addDefaultComponentById<IdVelocity>(registry, entity); }));
            expect(Ids::addDefaultComponentById<IdNoDefault>(registry, entity) == nullptr);
            expect(!registry.hasComponents<IdNoDefault>(entity));

            registry.killEntity(entity);
            expect(throws<std::invalid_argument>([&] { (void)Ids::addDefaultComponentById<IdNoDefault>(registry, entity); }));
        };
    }

    void test_remove_component_by_id() {
        "removeComponentById removes the component and fires onRemove"_test = [] {
            ECS::Registry registry;
            const auto entity = registry.spawnEntity();
            int removedX = 0;
            registry.onRemove<IdPosition>([&](IdPosition& position) { removedX = position.x; });
            registry.addComponent(entity, IdPosition{5, 6});

            Ids::removeComponentById(registry, entity, idOf<IdPosition>());

            expect(!registry.hasComponents<IdPosition>(entity));
            expect(removedX == 5_i);
            Ids::removeComponentById(registry, entity, idOf<IdVelocity>());   // no pool: no-op

            registry.killEntity(entity);
            expect(throws<std::invalid_argument>([&] { Ids::removeComponentById(registry, entity, idOf<IdPosition>()); }));
        };
    }

    void test_component_ids_of() {
        "componentIdsOf lists the owned components in ascending order"_test = [] {
            ECS::Registry registry;
            const auto entity = registry.spawnEntity();
            registry.addComponent(entity, IdVelocity{});
            registry.addComponent(entity, IdPosition{});

            std::vector<ECS::ComponentId> expected{idOf<IdPosition>(), idOf<IdVelocity>()};
            if (expected[0] > expected[1]) {
                std::swap(expected[0], expected[1]);
            }
            expect(Ids::componentIdsOf(registry, entity) == expected);

            registry.killEntity(entity);
            expect(Ids::componentIdsOf(registry, entity).empty());
        };
    }

    void test_deferred_add_default_component() {
        "a deferred addDefaultComponentById returns the staged value, applied at flush"_test = [] {
            ECS::Registry registry;
            const auto entity = registry.spawnEntity();
            registry.registerComponent<IdPosition>();
            bool visibleDuringDefer = true;

            registry.runDeferred([&] {
                auto* staged = static_cast<IdPosition*>(Ids::addDefaultComponentById<IdPosition>(registry, entity));
                staged->x = 42;
                visibleDuringDefer = registry.hasComponents<IdPosition>(entity);
            });

            expect(!visibleDuringDefer);
            expect(registry.getComponent<IdPosition>(entity).x == 42_i);
        };
    }

    void test_deferred_add_on_pending_and_killed_entities() {
        "a deferred addDefaultComponentById follows spawns and kills of the same batch"_test = [] {
            ECS::Registry registry;
            registry.registerComponent<IdPosition>();
            const auto doomed = registry.spawnEntity();
            ECS::Entity spawned{};

            registry.runDeferred([&] {
                spawned = registry.spawnEntity();
                (void)Ids::addDefaultComponentById<IdPosition>(registry, spawned);
                (void)Ids::addDefaultComponentById<IdPosition>(registry, doomed);
                registry.killEntity(doomed);
            });

            expect(registry.hasComponents<IdPosition>(spawned));
            expect(registry.getComponents<IdPosition>().size() == 1_ul);
        };
    }

    void test_deferred_remove_component_by_id() {
        "a deferred removeComponentById applies at flush, unless the entity died"_test = [] {
            ECS::Registry registry;
            const auto entity = registry.spawnEntity();
            const auto doomed = registry.spawnEntity();
            registry.addComponent(entity, IdPosition{});
            registry.addComponent(doomed, IdPosition{});
            bool hasDuringDefer = false;

            registry.runDeferred([&] {
                Ids::removeComponentById(registry, entity, idOf<IdPosition>());
                hasDuringDefer = registry.hasComponents<IdPosition>(entity);
                registry.killEntity(doomed);
                Ids::removeComponentById(registry, doomed, idOf<IdPosition>());
            });

            expect(hasDuringDefer);
            expect(!registry.hasComponents<IdPosition>(entity));
            expect(registry.getComponents<IdPosition>().size() == 0_ul);
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
    test_spawn_entity_stamps_registry_world();
    test_registries_have_distinct_world_ids();
    test_registry_rejects_entity_from_another_registry();
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
    test_on_remove_fires_via_remove_component();
    test_on_remove_fires_via_kill_entity();
    test_has_components_requires_all_requested_types();
    test_has_components_is_false_for_dead_entity();
    test_get_if_returns_null_for_unregistered_component();
    test_get_if_returns_registered_storage();
    test_registry_hands_out_read_only_storages();
    test_signature_mirrors_the_pools();
    test_view_returns_matching_entities();
    test_view_requires_all_requested_components();
    test_view_registers_unregistered_component_and_returns_no_matches();
    test_const_view_is_empty_for_unregistered_component();
    test_const_view_returns_const_components();
    test_set_entity_name_assigns_and_replaces_name();
    test_set_entity_name_rejects_dead_entity();
    test_get_entity_name_returns_empty_for_unknown_or_dead_entity();
    test_more_than_256_component_types_can_be_used();
    test_growing_signatures_keeps_existing_components();
    test_recycled_entity_owns_no_high_id_component();
    test_high_id_component_added_during_deferred_update();
    test_type_registered_in_another_registry_is_absent();
    test_set_resource_returns_stored_reference();
    test_has_resource_is_false_before_set();
    test_has_resource_is_true_after_set();
    test_get_resource_returns_the_set_value();
    test_get_resource_on_unset_type_throws();
    test_set_resource_twice_overwrites_previous_value();
    test_resources_are_independent_per_type();
    test_resource_and_component_ids_do_not_collide();
    test_get_resource_returns_mutable_reference();
    test_get_resource_is_const_accessible();
    test_const_get_resource_on_unset_type_throws();
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
    test_double_kill_in_same_deferred_batch_does_not_throw();
    test_add_component_after_kill_in_same_deferred_batch_is_a_no_op();
    test_emplace_component_after_kill_in_same_deferred_batch_is_a_no_op();
    test_remove_component_after_kill_in_same_deferred_batch_is_a_no_op();
    test_nested_run_deferred_applies_at_the_end_of_the_outer_call();
    test_ensure_pool_creates_once();
    test_has_components_by_id();
    test_get_component_by_id();
    test_add_default_component_immediate();
    test_add_default_component_replaces();
    test_add_default_component_errors();
    test_remove_component_by_id();
    test_component_ids_of();
    test_deferred_add_default_component();
    test_deferred_add_on_pending_and_killed_entities();
    test_deferred_remove_component_by_id();
    test_is_alive_is_true_for_a_spawned_entity();
    test_is_alive_is_false_after_kill();
    test_is_alive_rejects_foreign_default_and_stale_handles();
    test_is_alive_during_deferred_spawn_and_kill();
    test_entity_count_is_zero_for_a_new_registry();
    test_entity_count_follows_spawns_and_kills();
    test_entity_count_ignores_queued_spawns();
}
