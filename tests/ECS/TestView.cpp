#include <boost/ut.hpp>

#include <cstddef>
#include <tuple>
#include <type_traits>
#include <vector>

#include "ECS/ComponentStorage.hpp"
#include "ECS/EntityManager.hpp"
#include "ECS/View.hpp"

using namespace boost::ut;

namespace {
    struct Position {
        int value;
    };

    struct Velocity {
        int value;
    };

    struct Health {
        int value;
    };

    template<typename ViewType>
    std::vector<std::size_t> entityIds(ViewType& query) {
        std::vector<std::size_t> result;
        for (auto iterator = query.begin(); iterator != query.end(); ++iterator) {
            const auto values = *iterator;
            result.push_back(std::get<0>(values).index());
        }
        return result;
    }

    void test_view_is_empty_when_component_storage_is_empty() {
        "View is empty when its storage is empty"_test = [] {
            ECS::EntityManager manager;
            ECS::ComponentStorage<Position> positions;
            ECS::View<Position> query(manager, positions);

            expect(!(query.begin() != query.end()));
        };
    }

    void test_view_returns_entities_with_one_component() {
        "View returns entities with one component"_test = [] {
            ECS::EntityManager manager;
            const auto entity = manager.create();
            ECS::ComponentStorage<Position> positions;
            positions.emplaceAt(entity.index(), 10);
            ECS::View<Position> query(manager, positions);

            expect(entityIds(query) == std::vector<std::size_t>{entity.index()});
        };
    }

    void test_view_excludes_entities_without_required_component() {
        "View excludes entities without the required component"_test = [] {
            ECS::EntityManager manager;
            const auto first = manager.create();
            manager.create();
            ECS::ComponentStorage<Position> positions;
            positions.emplaceAt(first.index(), 10);
            ECS::View<Position> query(manager, positions);

            expect(entityIds(query).size() == std::size_t{1});
        };
    }

    void test_view_returns_multiple_matching_entities() {
        "View returns multiple matching entities"_test = [] {
            ECS::EntityManager manager;
            const auto first = manager.create();
            const auto second = manager.create();
            ECS::ComponentStorage<Position> positions;
            positions.emplaceAt(first.index(), 10);
            positions.emplaceAt(second.index(), 20);
            ECS::View<Position> query(manager, positions);

            expect(entityIds(query).size() == std::size_t{2});
        };
    }

    void test_view_returns_entity_first_in_tuple() {
        "View returns the entity first in its tuple"_test = [] {
            ECS::EntityManager manager;
            const auto entity = manager.create();
            ECS::ComponentStorage<Position> positions;
            positions.emplaceAt(entity.index(), 10);
            ECS::View<Position> query(manager, positions);
            using Value = decltype(*query.begin());

            static_assert(std::is_same_v<std::tuple_element_t<0, Value>, ECS::Entity>);
            expect(std::get<0>(*query.begin()) == entity);
        };
    }

    void test_view_returns_component_reference_after_entity() {
        "View returns the component after the entity"_test = [] {
            ECS::EntityManager manager;
            const auto entity = manager.create();
            ECS::ComponentStorage<Position> positions;
            positions.emplaceAt(entity.index(), 10);
            ECS::View<Position> query(manager, positions);
            using Value = decltype(*query.begin());

            static_assert(std::is_lvalue_reference_v<std::tuple_element_t<1, Value>>);
            expect(std::get<1>(*query.begin()).value == 10);
        };
    }

    void test_view_returns_correct_entity_handles() {
        "View returns current entity handles"_test = [] {
            ECS::EntityManager manager;
            const auto oldEntity = manager.create();
            manager.destroy(oldEntity);
            const auto currentEntity = manager.create();
            ECS::ComponentStorage<Position> positions;
            positions.emplaceAt(currentEntity.index(), 10);
            ECS::View<Position> query(manager, positions);

            expect(std::get<0>(*query.begin()) == currentEntity);
        };
    }

    void test_view_returns_only_intersection_of_two_components() {
        "View returns the intersection of two components"_test = [] {
            ECS::EntityManager manager;
            const auto first = manager.create();
            const auto second = manager.create();
            ECS::ComponentStorage<Position> positions;
            ECS::ComponentStorage<Velocity> velocities;
            positions.emplaceAt(first.index(), 1);
            positions.emplaceAt(second.index(), 2);
            velocities.emplaceAt(second.index(), 20);
            ECS::View<Position, Velocity> query(manager, positions, velocities);

            expect(entityIds(query) == std::vector<std::size_t>{second.index()});
        };
    }

    void test_view_returns_only_intersection_of_three_components() {
        "View returns the intersection of three components"_test = [] {
            ECS::EntityManager manager;
            const auto entity = manager.create();
            ECS::ComponentStorage<Position> positions;
            ECS::ComponentStorage<Velocity> velocities;
            ECS::ComponentStorage<Health> health;
            positions.emplaceAt(entity.index(), 1);
            velocities.emplaceAt(entity.index(), 2);
            health.emplaceAt(entity.index(), 3);
            ECS::View<Position, Velocity, Health> query(
                manager,
                positions,
                velocities,
                health
            );

            expect(entityIds(query) == std::vector<std::size_t>{entity.index()});
        };
    }

    void test_view_preserves_first_storage_order() {
        "View preserves first storage order"_test = [] {
            ECS::EntityManager manager;
            const auto first = manager.create();
            const auto second = manager.create();
            ECS::ComponentStorage<Position> positions;
            ECS::ComponentStorage<Velocity> velocities;
            positions.emplaceAt(second.index(), 2);
            positions.emplaceAt(first.index(), 1);
            velocities.emplaceAt(first.index(), 10);
            velocities.emplaceAt(second.index(), 20);
            ECS::View<Position, Velocity> query(manager, positions, velocities);

            expect(entityIds(query) ==
                   std::vector<std::size_t>{second.index(), first.index()});
        };
    }

    void test_view_allows_mutating_first_component() {
        "View allows mutating the first component"_test = [] {
            ECS::EntityManager manager;
            const auto entity = manager.create();
            ECS::ComponentStorage<Position> positions;
            positions.emplaceAt(entity.index(), 10);
            ECS::View<Position> query(manager, positions);

            std::get<1>(*query.begin()).value = 11;

            expect(positions.get(entity.index()).value == 11);
        };
    }

    void test_view_allows_mutating_second_component() {
        "View allows mutating the second component"_test = [] {
            ECS::EntityManager manager;
            const auto entity = manager.create();
            ECS::ComponentStorage<Position> positions;
            ECS::ComponentStorage<Velocity> velocities;
            positions.emplaceAt(entity.index(), 10);
            velocities.emplaceAt(entity.index(), 20);
            ECS::View<Position, Velocity> query(manager, positions, velocities);

            std::get<2>(*query.begin()).value = 21;

            expect(velocities.get(entity.index()).value == 21);
        };
    }

    void test_view_updates_all_matching_components() {
        "View updates all matching components"_test = [] {
            ECS::EntityManager manager;
            const auto first = manager.create();
            const auto second = manager.create();
            ECS::ComponentStorage<Position> positions;
            positions.emplaceAt(first.index(), 1);
            positions.emplaceAt(second.index(), 2);
            ECS::View<Position> query(manager, positions);

            for (auto iterator = query.begin(); iterator != query.end(); ++iterator) {
                ++std::get<1>(*iterator).value;
            }

            expect(positions.get(first.index()).value == 2);
            expect(positions.get(second.index()).value == 3);
        };
    }

    void test_view_reflects_storage_changes_before_begin() {
        "View reflects storage changes before begin"_test = [] {
            ECS::EntityManager manager;
            const auto entity = manager.create();
            ECS::ComponentStorage<Position> positions;
            ECS::View<Position> query(manager, positions);
            positions.emplaceAt(entity.index(), 10);

            expect(entityIds(query).size() == std::size_t{1});
        };
    }

    void test_view_reflects_removed_components_before_begin() {
        "View reflects removed components before begin"_test = [] {
            ECS::EntityManager manager;
            const auto entity = manager.create();
            ECS::ComponentStorage<Position> positions;
            positions.emplaceAt(entity.index(), 10);
            ECS::View<Position> query(manager, positions);
            positions.erase(entity.index());

            expect(!(query.begin() != query.end()));
        };
    }

    void test_read_only_view_returns_matching_entities() {
        "ReadOnlyView returns matching entities"_test = [] {
            ECS::EntityManager manager;
            const auto entity = manager.create();
            ECS::ComponentStorage<Position> positions;
            positions.emplaceAt(entity.index(), 10);
            const auto& constPositions = positions;
            ECS::ReadOnlyView<Position> query(manager, constPositions);

            expect(entityIds(query).size() == std::size_t{1});
        };
    }

    void test_read_only_view_returns_const_component_references() {
        "ReadOnlyView returns const component references"_test = [] {
            ECS::EntityManager manager;
            const auto entity = manager.create();
            ECS::ComponentStorage<Position> positions;
            positions.emplaceAt(entity.index(), 10);
            const auto& constPositions = positions;
            ECS::ReadOnlyView<Position> query(manager, constPositions);
            using Value = decltype(*query.begin());

            static_assert(std::is_const_v<
                std::remove_reference_t<std::tuple_element_t<1, Value>>
            >);
            expect(std::get<1>(*query.begin()).value == 10);
        };
    }

    void test_read_only_view_preserves_component_values() {
        "ReadOnlyView preserves component values"_test = [] {
            ECS::EntityManager manager;
            const auto entity = manager.create();
            ECS::ComponentStorage<Position> positions;
            positions.emplaceAt(entity.index(), 42);
            const auto& constPositions = positions;
            ECS::ReadOnlyView<Position> query(manager, constPositions);

            expect(std::get<1>(*query.begin()).value == 42);
        };
    }

    void test_const_registry_view_is_read_only() {
        "const registry view is read only"_test = [] {
            ECS::EntityManager manager;
            const auto entity = manager.create();
            ECS::ComponentStorage<Position> positions;
            positions.emplaceAt(entity.index(), 42);
            const auto& constPositions = positions;
            ECS::ReadOnlyView<Position> query(manager, constPositions);
            using Value = decltype(*query.begin());

            static_assert(std::is_const_v<
                std::remove_reference_t<std::tuple_element_t<1, Value>>
            >);
            expect(std::get<0>(*query.begin()) == entity);
        };
    }

    void test_view_iterator_increment_visits_all_matches() {
        "View iterator increment visits all matches"_test = [] {
            ECS::EntityManager manager;
            const auto first = manager.create();
            const auto second = manager.create();
            ECS::ComponentStorage<Position> positions;
            positions.emplaceAt(first.index(), 1);
            positions.emplaceAt(second.index(), 2);
            ECS::View<Position> query(manager, positions);
            auto iterator = query.begin();

            expect(std::get<0>(*iterator) == first);
            ++iterator;
            expect(std::get<0>(*iterator) == second);
            ++iterator;
            expect(!(iterator != query.end()));
        };
    }

    void test_view_iterator_stops_at_end() {
        "View iterator stops at end"_test = [] {
            ECS::EntityManager manager;
            const auto entity = manager.create();
            ECS::ComponentStorage<Position> positions;
            positions.emplaceAt(entity.index(), 1);
            ECS::View<Position> query(manager, positions);
            std::size_t count = 0;

            for (auto iterator = query.begin(); iterator != query.end(); ++iterator) {
                ++count;
            }

            expect(count == std::size_t{1});
        };
    }

    void test_view_with_sparse_entity_ids() {
        "View handles sparse entity ids"_test = [] {
            ECS::EntityManager manager;
            const auto first = manager.create();
            manager.create();
            const auto third = manager.create();
            ECS::ComponentStorage<Position> positions;
            positions.emplaceAt(third.index(), 30);
            positions.emplaceAt(first.index(), 10);
            ECS::View<Position> query(manager, positions);

            expect(entityIds(query) ==
                   std::vector<std::size_t>{third.index(), first.index()});
        };
    }

    void test_view_returns_current_generation_after_entity_reuse() {
        "View returns the current generation after entity reuse"_test = [] {
            ECS::EntityManager manager;
            const auto oldEntity = manager.create();
            manager.destroy(oldEntity);
            const auto newEntity = manager.create();
            ECS::ComponentStorage<Position> positions;
            positions.emplaceAt(newEntity.index(), 10);
            ECS::View<Position> query(manager, positions);

            expect(std::get<0>(*query.begin()) == newEntity);
            expect(std::get<0>(*query.begin()).generation() != oldEntity.generation());
        };
    }
}

void run_view_tests() {
    test_view_is_empty_when_component_storage_is_empty();
    test_view_returns_entities_with_one_component();
    test_view_excludes_entities_without_required_component();
    test_view_returns_multiple_matching_entities();
    test_view_returns_entity_first_in_tuple();
    test_view_returns_component_reference_after_entity();
    test_view_returns_correct_entity_handles();
    test_view_returns_only_intersection_of_two_components();
    test_view_returns_only_intersection_of_three_components();
    test_view_preserves_first_storage_order();
    test_view_allows_mutating_first_component();
    test_view_allows_mutating_second_component();
    test_view_updates_all_matching_components();
    test_view_reflects_storage_changes_before_begin();
    test_view_reflects_removed_components_before_begin();
    test_read_only_view_returns_matching_entities();
    test_read_only_view_returns_const_component_references();
    test_read_only_view_preserves_component_values();
    test_const_registry_view_is_read_only();
    test_view_iterator_increment_visits_all_matches();
    test_view_iterator_stops_at_end();
    test_view_with_sparse_entity_ids();
    test_view_returns_current_generation_after_entity_reuse();
}
