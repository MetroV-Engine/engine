#include <boost/ut.hpp>

#include <algorithm>
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <vector>

#include "ECS/ComponentStorage.hpp"
#include "ECS/EntityManager.hpp"
#include "ECS/Registry.hpp"
#include "ECS/View.hpp"
#include "ECS/Zipper.hpp"

using namespace boost::ut;

namespace {
    struct Position { int value; };
    struct Velocity { int value; };
    struct Health { int value; };

    /**
     * Test-only storage that forwards to a real ComponentStorage and counts
     * membership checks, so tests can assert how much work a query did
     * instead of relying on timing.
     */
    template<typename Component>
    class CountingStorage {
        public:
            using size_type = std::size_t;
            using value_type = Component;

            explicit CountingStorage(ECS::ComponentStorage<Component>& inner) : _inner(inner) {}

            [[nodiscard]] size_type size() const noexcept { return _inner.size(); }

            [[nodiscard]] bool has(size_type entityId) const noexcept {
                ++_hasCalls;
                return _inner.has(entityId);
            }

            Component& get(size_type entityId) { return _inner.get(entityId); }
            const Component& get(size_type entityId) const { return _inner.get(entityId); }

            [[nodiscard]] size_type entityAt(size_type packedIndex) const {
                return _inner.entityAt(packedIndex);
            }

            [[nodiscard]] const std::vector<size_type>& entities() const noexcept {
                return _inner.entities();
            }

            Component& operator[](size_type packedIndex) noexcept { return _inner[packedIndex]; }
            const Component& operator[](size_type packedIndex) const noexcept {
                return _inner[packedIndex];
            }

            [[nodiscard]] std::size_t hasCalls() const noexcept { return _hasCalls; }
            void resetCounts() noexcept { _hasCalls = 0; }

        private:
            ECS::ComponentStorage<Component>& _inner;
            mutable std::size_t _hasCalls{0};
    };

    template<typename Storage>
    void fill(Storage& storage, std::size_t firstId, std::size_t count) {
        for (std::size_t id = firstId; id < firstId + count; ++id) {
            storage.emplaceAt(id, static_cast<int>(id));
        }
    }

    template<typename Zipper>
    std::vector<std::size_t> zipperIds(Zipper& query) {
        std::vector<std::size_t> result;
        for (auto iterator = query.begin(); iterator != query.end(); ++iterator) {
            const auto values = *iterator;
            constexpr std::size_t last =
                std::tuple_size_v<std::decay_t<decltype(values)>> - 1;
            result.push_back(std::get<last>(values));
        }
        return result;
    }

    template<typename ViewType>
    std::vector<std::size_t> viewIds(ViewType& query) {
        std::vector<std::size_t> result;
        for (auto iterator = query.begin(); iterator != query.end(); ++iterator) {
            const auto values = *iterator;
            result.push_back(std::get<0>(values).index());
        }
        return result;
    }

    std::vector<std::size_t> sorted(std::vector<std::size_t> ids) {
        std::sort(ids.begin(), ids.end());
        return ids;
    }

    // --- Driver choice, observed through iteration order ---------------------
    // Iteration follows the driving storage's dense order, so the order of the
    // yielded ids tells which storage drove the loop.

    void test_smallest_second_storage_drives_iteration() {
        "smallest storage drives iteration when it is listed second"_test = [] {
            ECS::ComponentStorage<Position> positions;
            ECS::ComponentStorage<Velocity> velocities;
            positions.emplaceAt(30, 30);
            positions.emplaceAt(10, 10);
            positions.emplaceAt(20, 20);
            fill(velocities, 1, 40);
            auto query = ECS::zipper(velocities, positions);

            expect(zipperIds(query) == std::vector<std::size_t>{30, 10, 20});
        };
    }

    void test_smallest_first_storage_still_drives_iteration() {
        "smallest storage still drives iteration when it is listed first"_test = [] {
            ECS::ComponentStorage<Position> positions;
            ECS::ComponentStorage<Velocity> velocities;
            positions.emplaceAt(30, 30);
            positions.emplaceAt(10, 10);
            positions.emplaceAt(20, 20);
            fill(velocities, 1, 40);
            auto query = ECS::zipper(positions, velocities);

            expect(zipperIds(query) == std::vector<std::size_t>{30, 10, 20});
        };
    }

    void test_equal_sizes_first_listed_storage_drives() {
        "equal sizes let the first listed storage drive"_test = [] {
            ECS::ComponentStorage<Position> positions;
            ECS::ComponentStorage<Velocity> velocities;
            positions.emplaceAt(2, 2);
            positions.emplaceAt(1, 1);
            velocities.emplaceAt(1, 10);
            velocities.emplaceAt(2, 20);
            auto positionsFirst = ECS::zipper(positions, velocities);
            auto velocitiesFirst = ECS::zipper(velocities, positions);

            expect(zipperIds(positionsFirst) == std::vector<std::size_t>{2, 1});
            expect(zipperIds(velocitiesFirst) == std::vector<std::size_t>{1, 2});
        };
    }

    void test_three_storages_smallest_drives_in_any_position() {
        "smallest of three storages drives in first, middle and last position"_test = [] {
            ECS::ComponentStorage<Position> positions;
            ECS::ComponentStorage<Velocity> velocities;
            ECS::ComponentStorage<Health> health;
            fill(positions, 1, 20);
            fill(velocities, 1, 30);
            health.emplaceAt(7, 7);
            health.emplaceAt(3, 3);
            health.emplaceAt(5, 5);
            const std::vector<std::size_t> expected{7, 3, 5};

            auto healthFirst = ECS::zipper(health, positions, velocities);
            auto healthMiddle = ECS::zipper(positions, health, velocities);
            auto healthLast = ECS::zipper(positions, velocities, health);

            expect(zipperIds(healthFirst) == expected);
            expect(zipperIds(healthMiddle) == expected);
            expect(zipperIds(healthLast) == expected);
        };
    }

    void test_driver_is_chosen_at_begin_not_at_construction() {
        "driver is chosen when iteration begins, not when the zipper is built"_test = [] {
            ECS::ComponentStorage<Position> positions;
            ECS::ComponentStorage<Velocity> velocities;
            positions.emplaceAt(9, 9);
            positions.emplaceAt(4, 4);
            positions.emplaceAt(6, 6);
            auto query = ECS::zipper(velocities, positions);

            fill(velocities, 1, 20);

            expect(zipperIds(query) == std::vector<std::size_t>{9, 4, 6});
        };
    }

    void test_driver_is_reevaluated_between_iterations() {
        "driver is re-evaluated each time iteration begins"_test = [] {
            ECS::ComponentStorage<Position> positions;
            ECS::ComponentStorage<Velocity> velocities;
            positions.emplaceAt(3, 3);
            positions.emplaceAt(1, 1);
            positions.emplaceAt(2, 2);
            fill(velocities, 1, 10);
            auto query = ECS::zipper(positions, velocities);

            expect(zipperIds(query) == std::vector<std::size_t>{3, 1, 2});

            fill(positions, 4, 27);

            expect(zipperIds(query) ==
                   std::vector<std::size_t>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10});
        };
    }

    // --- Work done, observed through membership-check counts ----------------

    void test_membership_checks_follow_the_smallest_storage() {
        "membership checks scale with the smallest storage, not the first listed"_test = [] {
            ECS::ComponentStorage<Position> positions;
            ECS::ComponentStorage<Velocity> velocities;
            fill(positions, 0, 5);
            fill(velocities, 0, 10000);
            CountingStorage<Position> countedPositions(positions);
            CountingStorage<Velocity> countedVelocities(velocities);
            auto query = ECS::zipper(countedVelocities, countedPositions);

            const auto ids = zipperIds(query);

            expect(ids.size() == std::size_t{5});
            expect(countedVelocities.hasCalls() == std::size_t{5});
            expect(countedPositions.hasCalls() == std::size_t{0});
        };
    }

    void test_membership_checks_do_not_depend_on_argument_order() {
        "membership checks are the same whichever order storages are listed in"_test = [] {
            ECS::ComponentStorage<Position> positions;
            ECS::ComponentStorage<Velocity> velocities;
            fill(positions, 0, 5);
            fill(velocities, 0, 10000);
            CountingStorage<Position> countedPositions(positions);
            CountingStorage<Velocity> countedVelocities(velocities);
            auto query = ECS::zipper(countedPositions, countedVelocities);

            const auto ids = zipperIds(query);

            expect(ids.size() == std::size_t{5});
            expect(countedVelocities.hasCalls() == std::size_t{5});
            expect(countedPositions.hasCalls() == std::size_t{0});
        };
    }

    void test_membership_checks_with_three_storages_smallest_in_middle() {
        "three storages only check membership for the smallest storage's entities"_test = [] {
            ECS::ComponentStorage<Position> positions;
            ECS::ComponentStorage<Velocity> velocities;
            ECS::ComponentStorage<Health> health;
            fill(positions, 0, 10000);
            fill(velocities, 0, 5);
            fill(health, 0, 500);
            CountingStorage<Position> countedPositions(positions);
            CountingStorage<Velocity> countedVelocities(velocities);
            CountingStorage<Health> countedHealth(health);
            auto query = ECS::zipper(countedPositions, countedVelocities, countedHealth);

            const auto ids = zipperIds(query);

            expect(ids.size() == std::size_t{5});
            expect(countedVelocities.hasCalls() == std::size_t{0});
            expect(countedPositions.hasCalls() == std::size_t{5});
            expect(countedHealth.hasCalls() == std::size_t{5});
        };
    }

    void test_empty_storage_means_no_membership_checks() {
        "an empty storage anywhere in the query means no membership checks at all"_test = [] {
            ECS::ComponentStorage<Position> positions;
            ECS::ComponentStorage<Velocity> velocities;
            fill(velocities, 0, 10000);
            CountingStorage<Position> countedPositions(positions);
            CountingStorage<Velocity> countedVelocities(velocities);
            auto query = ECS::zipper(countedVelocities, countedPositions);

            const auto ids = zipperIds(query);

            expect(ids.empty());
            expect(countedVelocities.hasCalls() == std::size_t{0});
            expect(countedPositions.hasCalls() == std::size_t{0});
        };
    }

    void test_membership_checks_follow_driver_after_sizes_change() {
        "membership checks follow the new smallest storage after sizes change"_test = [] {
            ECS::ComponentStorage<Position> positions;
            ECS::ComponentStorage<Velocity> velocities;
            fill(positions, 0, 5);
            fill(velocities, 0, 100);
            CountingStorage<Position> countedPositions(positions);
            CountingStorage<Velocity> countedVelocities(velocities);
            auto query = ECS::zipper(countedPositions, countedVelocities);
            (void)zipperIds(query);

            fill(positions, 5, 995);
            countedPositions.resetCounts();
            countedVelocities.resetCounts();
            const auto ids = zipperIds(query);

            expect(ids.size() == std::size_t{100});
            expect(countedPositions.hasCalls() == std::size_t{100});
            expect(countedVelocities.hasCalls() == std::size_t{0});
        };
    }

    // --- Correctness regardless of which storage drives ----------------------

    void test_same_entities_for_every_argument_order() {
        "every argument order yields the same set of entities"_test = [] {
            ECS::ComponentStorage<Position> positions;
            ECS::ComponentStorage<Velocity> velocities;
            ECS::ComponentStorage<Health> health;
            for (std::size_t id : {1, 2, 3, 5, 8, 13}) { positions.emplaceAt(id, 0); }
            for (std::size_t id : {2, 3, 5, 7, 11, 13, 17}) { velocities.emplaceAt(id, 0); }
            for (std::size_t id : {3, 5, 13, 21}) { health.emplaceAt(id, 0); }
            const std::vector<std::size_t> expected{3, 5, 13};

            auto pvh = ECS::zipper(positions, velocities, health);
            auto phv = ECS::zipper(positions, health, velocities);
            auto vph = ECS::zipper(velocities, positions, health);
            auto vhp = ECS::zipper(velocities, health, positions);
            auto hpv = ECS::zipper(health, positions, velocities);
            auto hvp = ECS::zipper(health, velocities, positions);

            expect(sorted(zipperIds(pvh)) == expected);
            expect(sorted(zipperIds(phv)) == expected);
            expect(sorted(zipperIds(vph)) == expected);
            expect(sorted(zipperIds(vhp)) == expected);
            expect(sorted(zipperIds(hpv)) == expected);
            expect(sorted(zipperIds(hvp)) == expected);
        };
    }

    void test_tuple_keeps_argument_order_when_driver_is_not_first() {
        "yielded tuple keeps argument order when the driver is not listed first"_test = [] {
            ECS::ComponentStorage<Position> positions;
            ECS::ComponentStorage<Velocity> velocities;
            fill(velocities, 0, 10);
            velocities.get(4).value = 400;
            positions.emplaceAt(4, 40);
            auto query = ECS::zipper(velocities, positions);
            using Values = std::decay_t<decltype(*query.begin())>;

            static_assert(std::is_same_v<std::tuple_element_t<0, Values>, Velocity&>);
            static_assert(std::is_same_v<std::tuple_element_t<1, Values>, Position&>);

            const auto values = *query.begin();
            expect(std::get<0>(values).value == 400);
            expect(std::get<1>(values).value == 40);
            expect(std::get<2>(values) == std::size_t{4});
        };
    }

    void test_every_component_belongs_to_the_yielded_entity() {
        "every yielded component belongs to the yielded entity"_test = [] {
            ECS::ComponentStorage<Position> positions;
            ECS::ComponentStorage<Velocity> velocities;
            ECS::ComponentStorage<Health> health;
            for (std::size_t id = 0; id < 50; ++id) {
                velocities.emplaceAt(id, static_cast<int>(id * 100));
                health.emplaceAt(id, static_cast<int>(id * 1000));
            }
            for (std::size_t id : {42, 7, 19}) {
                positions.emplaceAt(id, static_cast<int>(id * 10));
            }
            auto query = ECS::zipper(velocities, health, positions);

            std::size_t visited = 0;
            for (auto iterator = query.begin(); iterator != query.end(); ++iterator) {
                const auto [velocity, hp, position, id] = *iterator;
                expect(velocity.value == static_cast<int>(id * 100));
                expect(hp.value == static_cast<int>(id * 1000));
                expect(position.value == static_cast<int>(id * 10));
                ++visited;
            }
            expect(visited == std::size_t{3});
        };
    }

    void test_component_references_are_mutable_when_driver_is_not_first() {
        "yielded references write through when the driver is not listed first"_test = [] {
            ECS::ComponentStorage<Position> positions;
            ECS::ComponentStorage<Velocity> velocities;
            fill(velocities, 0, 10);
            positions.emplaceAt(3, 30);
            auto query = ECS::zipper(velocities, positions);

            for (auto iterator = query.begin(); iterator != query.end(); ++iterator) {
                auto values = *iterator;
                std::get<0>(values).value = -1;
                std::get<1>(values).value = -2;
            }

            expect(velocities.get(3).value == -1);
            expect(positions.get(3).value == -2);
            expect(velocities.get(2).value == 2);
        };
    }

    void test_empty_intersection_between_non_empty_storages() {
        "non empty storages with no shared entity yield nothing"_test = [] {
            ECS::ComponentStorage<Position> positions;
            ECS::ComponentStorage<Velocity> velocities;
            fill(positions, 0, 3);
            fill(velocities, 100, 50);
            auto query = ECS::zipper(velocities, positions);

            expect(zipperIds(query).empty());
        };
    }

    void test_empty_storage_in_any_position_yields_nothing() {
        "an empty storage in any position yields nothing"_test = [] {
            ECS::ComponentStorage<Position> positions;
            ECS::ComponentStorage<Velocity> velocities;
            ECS::ComponentStorage<Health> empty;
            fill(positions, 0, 10);
            fill(velocities, 0, 10);

            auto emptyFirst = ECS::zipper(empty, positions, velocities);
            auto emptyMiddle = ECS::zipper(positions, empty, velocities);
            auto emptyLast = ECS::zipper(positions, velocities, empty);

            expect(zipperIds(emptyFirst).empty());
            expect(zipperIds(emptyMiddle).empty());
            expect(zipperIds(emptyLast).empty());
        };
    }

    void test_large_sparse_ids_when_driver_is_not_first() {
        "large sparse ids resolve correctly when the driver is not listed first"_test = [] {
            ECS::ComponentStorage<Position> positions;
            ECS::ComponentStorage<Velocity> velocities;
            for (std::size_t id : {5, 70000, 100000, 3}) {
                velocities.emplaceAt(id, static_cast<int>(id));
            }
            positions.emplaceAt(100000, 1);
            positions.emplaceAt(5, 2);
            auto query = ECS::zipper(velocities, positions);

            expect(zipperIds(query) == std::vector<std::size_t>{100000, 5});
        };
    }

    void test_driver_order_after_swap_and_pop() {
        "driver order reflects a swap and pop removal"_test = [] {
            ECS::ComponentStorage<Position> positions;
            ECS::ComponentStorage<Velocity> velocities;
            positions.emplaceAt(1, 1);
            positions.emplaceAt(2, 2);
            positions.emplaceAt(3, 3);
            fill(velocities, 0, 10);
            positions.erase(1);
            auto query = ECS::zipper(velocities, positions);

            expect(zipperIds(query) == std::vector<std::size_t>{3, 2});
        };
    }

    void test_same_storage_twice_still_matches() {
        "the same storage listed twice yields its own entities"_test = [] {
            ECS::ComponentStorage<Position> positions;
            positions.emplaceAt(4, 4);
            positions.emplaceAt(2, 2);
            auto query = ECS::zipper(positions, positions);

            expect(zipperIds(query) == std::vector<std::size_t>{4, 2});
        };
    }

    // --- View ----------------------------------------------------------------

    void test_view_follows_smallest_storage() {
        "View iterates in the smallest storage's order with correct handles"_test = [] {
            ECS::EntityManager manager;
            std::vector<ECS::Entity> entities;
            for (int i = 0; i < 10; ++i) { entities.push_back(manager.create()); }
            ECS::ComponentStorage<Position> positions;
            ECS::ComponentStorage<Velocity> velocities;
            for (const auto entity : entities) {
                velocities.emplaceAt(entity.index(), static_cast<int>(entity.index()));
            }
            positions.emplaceAt(entities[7].index(), 70);
            positions.emplaceAt(entities[2].index(), 20);
            ECS::View<Velocity, Position> query(manager, velocities, positions);

            expect(viewIds(query) ==
                   std::vector<std::size_t>{entities[7].index(), entities[2].index()});
            for (auto [entity, velocity, position] : query) {
                expect(entity == manager.entityFromIndex(entity.index()));
                expect(velocity.value == static_cast<int>(entity.index()));
                expect(position.value == static_cast<int>(entity.index() * 10));
            }
        };
    }

    void test_read_only_view_follows_smallest_storage() {
        "ReadOnlyView iterates in the smallest storage's order"_test = [] {
            ECS::EntityManager manager;
            std::vector<ECS::Entity> entities;
            for (int i = 0; i < 10; ++i) { entities.push_back(manager.create()); }
            ECS::ComponentStorage<Position> positions;
            ECS::ComponentStorage<Velocity> velocities;
            for (const auto entity : entities) {
                velocities.emplaceAt(entity.index(), 0);
            }
            positions.emplaceAt(entities[5].index(), 0);
            positions.emplaceAt(entities[1].index(), 0);
            const auto& constVelocities = velocities;
            const auto& constPositions = positions;
            ECS::ReadOnlyView<Velocity, Position> query(manager, constVelocities, constPositions);

            expect(viewIds(query) ==
                   std::vector<std::size_t>{entities[5].index(), entities[1].index()});
        };
    }

    void test_view_driver_reflects_changes_before_begin() {
        "View picks the driver from storage sizes at begin"_test = [] {
            ECS::EntityManager manager;
            std::vector<ECS::Entity> entities;
            for (int i = 0; i < 10; ++i) { entities.push_back(manager.create()); }
            ECS::ComponentStorage<Position> positions;
            ECS::ComponentStorage<Velocity> velocities;
            positions.emplaceAt(entities[8].index(), 0);
            positions.emplaceAt(entities[4].index(), 0);
            ECS::View<Velocity, Position> query(manager, velocities, positions);

            for (const auto entity : entities) {
                velocities.emplaceAt(entity.index(), 0);
            }

            expect(viewIds(query) ==
                   std::vector<std::size_t>{entities[8].index(), entities[4].index()});
        };
    }

    // --- Registry ------------------------------------------------------------

    void test_registry_view_follows_smallest_storage() {
        "Registry view iterates the smallest storage and writes through"_test = [] {
            ECS::Registry registry;
            std::vector<ECS::Entity> entities;
            for (int i = 0; i < 20; ++i) {
                const auto entity = registry.spawnEntity();
                registry.emplaceComponent<Velocity>(entity, i);
                entities.push_back(entity);
            }
            registry.emplaceComponent<Position>(entities[15], 0);
            registry.emplaceComponent<Position>(entities[3], 0);
            registry.emplaceComponent<Position>(entities[9], 0);

            std::vector<std::size_t> seen;
            for (auto [entity, velocity, position] : registry.view<Velocity, Position>()) {
                seen.push_back(entity.index());
                position.value = velocity.value * 2;
            }

            expect(seen == std::vector<std::size_t>{
                entities[15].index(), entities[3].index(), entities[9].index()});
            expect(registry.getComponent<Position>(entities[15]).value == 30);
            expect(registry.getComponent<Position>(entities[3]).value == 6);
            expect(registry.getComponent<Position>(entities[9]).value == 18);
        };
    }

    void test_const_registry_view_follows_smallest_storage() {
        "const Registry view iterates the smallest storage"_test = [] {
            ECS::Registry registry;
            std::vector<ECS::Entity> entities;
            for (int i = 0; i < 20; ++i) {
                const auto entity = registry.spawnEntity();
                registry.emplaceComponent<Velocity>(entity, i);
                entities.push_back(entity);
            }
            registry.emplaceComponent<Position>(entities[11], 0);
            registry.emplaceComponent<Position>(entities[6], 0);
            const ECS::Registry& constRegistry = registry;

            std::vector<std::size_t> seen;
            for (auto [entity, velocity, position] : constRegistry.view<Velocity, Position>()) {
                (void)velocity;
                (void)position;
                seen.push_back(entity.index());
            }

            expect(seen == std::vector<std::size_t>{entities[11].index(), entities[6].index()});
        };
    }

    void test_registry_view_inside_deferred_run() {
        "Registry view driven by the smallest storage stays intact under deferred removals"_test = [] {
            ECS::Registry registry;
            std::vector<ECS::Entity> entities;
            for (int i = 0; i < 20; ++i) {
                const auto entity = registry.spawnEntity();
                registry.emplaceComponent<Velocity>(entity, i);
                entities.push_back(entity);
            }
            registry.emplaceComponent<Position>(entities[12], 0);
            registry.emplaceComponent<Position>(entities[1], 0);
            registry.emplaceComponent<Position>(entities[17], 0);

            std::vector<std::size_t> seen;
            registry.runDeferred([&] {
                for (auto [entity, velocity, position] : registry.view<Velocity, Position>()) {
                    (void)velocity;
                    (void)position;
                    seen.push_back(entity.index());
                    registry.removeComponent<Position>(entity);
                }
            });

            expect(seen == std::vector<std::size_t>{
                entities[12].index(), entities[1].index(), entities[17].index()});
            expect(!registry.hasComponents<Position>(entities[12]));
            expect(!registry.hasComponents<Position>(entities[1]));
            expect(!registry.hasComponents<Position>(entities[17]));
        };
    }
}

void run_zipper_driver_tests() {
    test_smallest_second_storage_drives_iteration();
    test_smallest_first_storage_still_drives_iteration();
    test_equal_sizes_first_listed_storage_drives();
    test_three_storages_smallest_drives_in_any_position();
    test_driver_is_chosen_at_begin_not_at_construction();
    test_driver_is_reevaluated_between_iterations();
    test_membership_checks_follow_the_smallest_storage();
    test_membership_checks_do_not_depend_on_argument_order();
    test_membership_checks_with_three_storages_smallest_in_middle();
    test_empty_storage_means_no_membership_checks();
    test_membership_checks_follow_driver_after_sizes_change();
    test_same_entities_for_every_argument_order();
    test_tuple_keeps_argument_order_when_driver_is_not_first();
    test_every_component_belongs_to_the_yielded_entity();
    test_component_references_are_mutable_when_driver_is_not_first();
    test_empty_intersection_between_non_empty_storages();
    test_empty_storage_in_any_position_yields_nothing();
    test_large_sparse_ids_when_driver_is_not_first();
    test_driver_order_after_swap_and_pop();
    test_same_storage_twice_still_matches();
    test_view_follows_smallest_storage();
    test_read_only_view_follows_smallest_storage();
    test_view_driver_reflects_changes_before_begin();
    test_registry_view_follows_smallest_storage();
    test_const_registry_view_follows_smallest_storage();
    test_registry_view_inside_deferred_run();
}
