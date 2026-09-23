#include <boost/ut.hpp>

#include <cstddef>
#include <tuple>
#include <vector>

#include "ECS/ComponentStorage.hpp"
#include "ECS/Zipper.hpp"

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

    template<typename Zipper>
    std::vector<std::size_t> entityIds(Zipper& query) {
        std::vector<std::size_t> result;
        for (auto iterator = query.begin(); iterator != query.end(); ++iterator) {
            const auto values = *iterator;
            result.push_back(std::get<2>(values));
        }
        return result;
    }

    void test_zipper_can_be_created_with_multiple_storages() {
        "Zipper can be created with multiple storages"_test = [] {
            ECS::ComponentStorage<Position> positions;
            ECS::ComponentStorage<Velocity> velocities;
            auto query = ECS::zipper(positions, velocities);

            expect(!(query.begin() != query.end()));
        };
    }

    void test_zipper_is_empty_when_first_storage_is_empty() {
        "Zipper is empty when the first storage is empty"_test = [] {
            ECS::ComponentStorage<Position> positions;
            ECS::ComponentStorage<Velocity> velocities;
            velocities.emplaceAt(1, 2);
            auto query = ECS::zipper(positions, velocities);

            expect(!(query.begin() != query.end()));
        };
    }

    void test_zipper_is_empty_when_other_storage_is_empty() {
        "Zipper is empty when another storage is empty"_test = [] {
            ECS::ComponentStorage<Position> positions;
            ECS::ComponentStorage<Velocity> velocities;
            positions.emplaceAt(1, 1);
            auto query = ECS::zipper(positions, velocities);

            expect(!(query.begin() != query.end()));
        };
    }

    void test_zipper_returns_common_entity() {
        "Zipper returns a common entity"_test = [] {
            ECS::ComponentStorage<Position> positions;
            ECS::ComponentStorage<Velocity> velocities;
            positions.emplaceAt(3, 10);
            velocities.emplaceAt(3, 20);
            auto query = ECS::zipper(positions, velocities);

            const auto values = *query.begin();
            expect(std::get<0>(values).value == 10);
            expect(std::get<1>(values).value == 20);
            expect(std::get<2>(values) == std::size_t{3});
        };
    }

    void test_zipper_excludes_entity_missing_from_second_storage() {
        "Zipper excludes entities missing from the second storage"_test = [] {
            ECS::ComponentStorage<Position> positions;
            ECS::ComponentStorage<Velocity> velocities;
            positions.emplaceAt(1, 10);
            velocities.emplaceAt(2, 20);
            auto query = ECS::zipper(positions, velocities);

            expect(!(query.begin() != query.end()));
        };
    }

    void test_zipper_excludes_entity_missing_from_third_storage() {
        "Zipper excludes entities missing from the third storage"_test = [] {
            ECS::ComponentStorage<Position> positions;
            ECS::ComponentStorage<Velocity> velocities;
            ECS::ComponentStorage<Health> health;
            positions.emplaceAt(1, 10);
            velocities.emplaceAt(1, 20);
            health.emplaceAt(2, 30);
            auto query = ECS::zipper(positions, velocities, health);

            expect(!(query.begin() != query.end()));
        };
    }

    void test_zipper_returns_only_the_intersection() {
        "Zipper returns only the storage intersection"_test = [] {
            ECS::ComponentStorage<Position> positions;
            ECS::ComponentStorage<Velocity> velocities;
            positions.emplaceAt(1, 1);
            positions.emplaceAt(2, 2);
            positions.emplaceAt(3, 3);
            velocities.emplaceAt(2, 20);
            velocities.emplaceAt(3, 30);
            auto query = ECS::zipper(positions, velocities);

            expect(entityIds(query) == std::vector<std::size_t>{2, 3});
        };
    }

    void test_zipper_returns_multiple_common_entities() {
        "Zipper returns multiple common entities"_test = [] {
            ECS::ComponentStorage<Position> positions;
            ECS::ComponentStorage<Velocity> velocities;
            positions.emplaceAt(1, 1);
            positions.emplaceAt(2, 2);
            velocities.emplaceAt(1, 10);
            velocities.emplaceAt(2, 20);
            auto query = ECS::zipper(positions, velocities);

            expect(entityIds(query).size() == std::size_t{2});
        };
    }

    void test_zipper_handles_sparse_entity_ids() {
        "Zipper handles sparse entity ids"_test = [] {
            ECS::ComponentStorage<Position> positions;
            ECS::ComponentStorage<Velocity> velocities;
            positions.emplaceAt(2, 2);
            positions.emplaceAt(100, 100);
            velocities.emplaceAt(100, 1000);
            auto query = ECS::zipper(positions, velocities);

            expect(entityIds(query) == std::vector<std::size_t>{100});
        };
    }

    void test_zipper_preserves_first_storage_dense_order() {
        "Zipper preserves the first storage dense order"_test = [] {
            ECS::ComponentStorage<Position> positions;
            ECS::ComponentStorage<Velocity> velocities;
            positions.emplaceAt(5, 5);
            positions.emplaceAt(1, 1);
            velocities.emplaceAt(5, 50);
            velocities.emplaceAt(1, 10);
            auto query = ECS::zipper(positions, velocities);

            expect(entityIds(query) == std::vector<std::size_t>{5, 1});
        };
    }

    void test_zipper_skips_non_matching_entities_between_matches() {
        "Zipper skips non matching entities between matches"_test = [] {
            ECS::ComponentStorage<Position> positions;
            ECS::ComponentStorage<Velocity> velocities;
            positions.emplaceAt(1, 1);
            positions.emplaceAt(2, 2);
            positions.emplaceAt(3, 3);
            velocities.emplaceAt(1, 10);
            velocities.emplaceAt(3, 30);
            auto query = ECS::zipper(positions, velocities);

            expect(entityIds(query) == std::vector<std::size_t>{1, 3});
        };
    }

    void test_zipper_returns_component_references() {
        "Zipper returns references to matching components"_test = [] {
            ECS::ComponentStorage<Position> positions;
            ECS::ComponentStorage<Velocity> velocities;
            positions.emplaceAt(1, 10);
            velocities.emplaceAt(1, 20);
            auto query = ECS::zipper(positions, velocities);
            auto values = *query.begin();

            std::get<0>(values).value = 11;
            std::get<1>(values).value = 21;

            expect(positions.get(1).value == 11);
            expect(velocities.get(1).value == 21);
        };
    }

    void test_zipper_returns_correct_entity_id() {
        "Zipper returns the correct entity id"_test = [] {
            ECS::ComponentStorage<Position> positions;
            ECS::ComponentStorage<Velocity> velocities;
            positions.emplaceAt(7, 1);
            velocities.emplaceAt(7, 2);
            auto query = ECS::zipper(positions, velocities);

            expect(std::get<2>(*query.begin()) == std::size_t{7});
        };
    }

    void test_zipper_increment_reaches_end() {
        "Zipper increment reaches end"_test = [] {
            ECS::ComponentStorage<Position> positions;
            ECS::ComponentStorage<Velocity> velocities;
            positions.emplaceAt(1, 1);
            velocities.emplaceAt(1, 2);
            auto query = ECS::zipper(positions, velocities);
            auto iterator = query.begin();

            ++iterator;

            expect(!(iterator != query.end()));
        };
    }

    void test_zipper_with_same_storage_twice_requires_same_entity() {
        "Zipper with the same storage twice keeps matching entities"_test = [] {
            ECS::ComponentStorage<Position> positions;
            positions.emplaceAt(4, 4);
            auto query = ECS::zipper(positions, positions);

            expect(entityIds(query) == std::vector<std::size_t>{4});
        };
    }

    void test_zipper_reflects_storage_changes_before_iteration() {
        "Zipper reflects changes before iteration"_test = [] {
            ECS::ComponentStorage<Position> positions;
            ECS::ComponentStorage<Velocity> velocities;
            auto query = ECS::zipper(positions, velocities);
            positions.emplaceAt(9, 1);
            velocities.emplaceAt(9, 2);

            expect(entityIds(query) == std::vector<std::size_t>{9});
        };
    }

    void test_zipper_does_not_return_removed_entities() {
        "Zipper does not return removed entities"_test = [] {
            ECS::ComponentStorage<Position> positions;
            ECS::ComponentStorage<Velocity> velocities;
            positions.emplaceAt(1, 1);
            velocities.emplaceAt(1, 2);
            auto query = ECS::zipper(positions, velocities);
            velocities.erase(1);

            expect(!(query.begin() != query.end()));
        };
    }

    void test_zipper_updates_after_swap_and_pop() {
        "Zipper remains correct after swap and pop"_test = [] {
            ECS::ComponentStorage<Position> positions;
            ECS::ComponentStorage<Velocity> velocities;
            positions.emplaceAt(1, 1);
            positions.emplaceAt(2, 2);
            velocities.emplaceAt(1, 10);
            velocities.emplaceAt(2, 20);
            positions.erase(1);
            auto query = ECS::zipper(positions, velocities);

            expect(entityIds(query) == std::vector<std::size_t>{2});
        };
    }
}

void run_zipper_tests() {
    test_zipper_can_be_created_with_multiple_storages();
    test_zipper_is_empty_when_first_storage_is_empty();
    test_zipper_is_empty_when_other_storage_is_empty();
    test_zipper_returns_common_entity();
    test_zipper_excludes_entity_missing_from_second_storage();
    test_zipper_excludes_entity_missing_from_third_storage();
    test_zipper_returns_only_the_intersection();
    test_zipper_returns_multiple_common_entities();
    test_zipper_handles_sparse_entity_ids();
    test_zipper_preserves_first_storage_dense_order();
    test_zipper_skips_non_matching_entities_between_matches();
    test_zipper_returns_component_references();
    test_zipper_returns_correct_entity_id();
    test_zipper_increment_reaches_end();
    test_zipper_with_same_storage_twice_requires_same_entity();
    test_zipper_reflects_storage_changes_before_iteration();
    test_zipper_does_not_return_removed_entities();
    test_zipper_updates_after_swap_and_pop();
}
