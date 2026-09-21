#include <boost/ut.hpp>

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "ECS/ComponentStorage.hpp"

using namespace boost::ut;

namespace {
    struct Position {
        int x;
        int y;
    };

    struct Health {
        int value;
    };

    struct Empty {};

    struct MoveOnly {
        MoveOnly(int value) : value(value) {}
        MoveOnly(const MoveOnly&) = delete;
        MoveOnly& operator=(const MoveOnly&) = delete;
        MoveOnly(MoveOnly&&) = default;
        MoveOnly& operator=(MoveOnly&&) = default;
        int value;
    };

    template<typename Type>
    struct TestAllocator : std::allocator<Type> {
        using std::allocator<Type>::allocator;

        template<typename Other>
        struct rebind {
            using other = TestAllocator<Other>;
        };
    };

    template<typename Storage>
    std::vector<int> values(const Storage& storage) {
        std::vector<int> result;
        for (const auto& component : storage) {
            result.push_back(component.value);
        }
        return result;
    }
}

namespace {
void test_storage_is_empty_by_default() {
    "ComponentStorage is empty by default"_test = [] {
        const ECS::ComponentStorage<Health> storage;

        expect(storage.size() == std::size_t{0});
        expect(storage.begin() == storage.end());
    };
}

void test_storage_has_no_component_by_default() {
    "ComponentStorage has no component by default"_test = [] {
        const ECS::ComponentStorage<Health> storage;

        expect(!storage.has(0));
        expect(!storage.has(100));
    };
}

void test_storage_npos_is_maximum_size() {
    "ComponentStorage npos is the maximum size"_test = [] {
        expect(ECS::ComponentStorage<Health>::npos ==
               static_cast<std::size_t>(-1));
    };
}

void test_reserve_does_not_change_size() {
    "reserve does not change storage size"_test = [] {
        ECS::ComponentStorage<Health> storage;
        storage.reserve(10, 20);

        expect(storage.size() == std::size_t{0});
        expect(!storage.has(0));
    };
}

void test_reserve_supports_later_insertion() {
    "reserve supports later insertion"_test = [] {
        ECS::ComponentStorage<Health> storage;
        storage.reserve(2, 10);
        storage.emplaceAt(3, 42);

        expect(storage.get(3).value == 42);
    };
}

void test_multiple_reserve_calls_preserve_components() {
    "multiple reserve calls preserve components"_test = [] {
        ECS::ComponentStorage<Health> storage;
        storage.emplaceAt(1, 42);
        storage.reserve(10, 20);

        expect(storage.get(1).value == 42);
    };
}

void test_reserve_entity_capacity_supports_sparse_ids() {
    "reserve supports sparse entity ids"_test = [] {
        ECS::ComponentStorage<Health> storage;
        storage.reserve(1, 100);
        storage.emplaceAt(99, 7);

        expect(storage.has(99));
        expect(storage.get(99).value == 7);
    };
}

void test_insert_at_copy_adds_component() {
    "insertAt copy adds a component"_test = [] {
        ECS::ComponentStorage<Position> storage;
        const Position position{10, 20};
        auto& stored = storage.insertAt(2, position);

        expect(storage.size() == std::size_t{1});
        expect(storage.has(2));
        expect(stored.x == 10);
        expect(storage.get(2).y == 20);
    };
}

void test_insert_at_copy_returns_reference_to_stored_component() {
    "insertAt copy returns the stored reference"_test = [] {
        ECS::ComponentStorage<Position> storage;
        auto& stored = storage.insertAt(2, Position{1, 2});
        stored.x = 9;

        expect(storage.get(2).x == 9);
    };
}

void test_insert_at_copy_copies_value() {
    "insertAt copy owns a copy of the value"_test = [] {
        ECS::ComponentStorage<Position> storage;
        Position source{1, 2};
        storage.insertAt(2, source);
        source.x = 99;

        expect(storage.get(2).x == 1);
    };
}

void test_insert_at_copy_supports_sparse_entity_ids() {
    "insertAt copy supports sparse ids"_test = [] {
        ECS::ComponentStorage<Health> storage;
        storage.insertAt(50, Health{9});

        expect(storage.size() == std::size_t{1});
        expect(storage.has(50));
        expect(!storage.has(49));
    };
}

void test_insert_at_copy_replaces_existing_component() {
    "insertAt copy replaces an existing component"_test = [] {
        ECS::ComponentStorage<Health> storage;
        storage.insertAt(1, Health{1});
        storage.insertAt(1, Health{2});

        expect(storage.get(1).value == 2);
    };
}

void test_insert_at_copy_replacement_does_not_increase_size() {
    "insertAt copy replacement keeps the size"_test = [] {
        ECS::ComponentStorage<Health> storage;
        storage.insertAt(1, Health{1});
        storage.insertAt(1, Health{2});

        expect(storage.size() == std::size_t{1});
    };
}

void test_insert_at_move_adds_component() {
    "insertAt move adds a component"_test = [] {
        ECS::ComponentStorage<Health> storage;
        storage.insertAt(3, Health{42});

        expect(storage.get(3).value == 42);
    };
}

void test_insert_at_move_stores_the_moved_value() {
    "insertAt move stores the moved value"_test = [] {
        ECS::ComponentStorage<Health> storage;
        Health source{42};
        storage.insertAt(3, std::move(source));

        expect(storage.get(3).value == 42);
    };
}

void test_insert_at_move_replaces_existing_component() {
    "insertAt move replaces an existing component"_test = [] {
        ECS::ComponentStorage<Health> storage;
        storage.insertAt(3, Health{1});
        storage.insertAt(3, Health{42});

        expect(storage.size() == std::size_t{1});
        expect(storage.get(3).value == 42);
    };
}

void test_insert_at_move_replacement_does_not_increase_size() {
    "insertAt move replacement keeps the size"_test = [] {
        ECS::ComponentStorage<Health> storage;
        storage.insertAt(3, Health{1});
        storage.insertAt(3, Health{2});

        expect(storage.size() == std::size_t{1});
    };
}

void test_insert_at_move_supports_move_only_components() {
    "insertAt move supports move-only components"_test = [] {
        ECS::ComponentStorage<MoveOnly> storage;
        storage.insertAt(3, MoveOnly{42});

        expect(storage.get(3).value == 42);
    };
}

void test_emplace_at_constructs_component() {
    "emplaceAt constructs a component"_test = [] {
        ECS::ComponentStorage<Position> storage;
        storage.emplaceAt(4, 10, 20);

        expect(storage.get(4).x == 10);
        expect(storage.get(4).y == 20);
    };
}

void test_emplace_at_forwards_constructor_arguments() {
    "emplaceAt forwards constructor arguments"_test = [] {
        ECS::ComponentStorage<std::string> storage;
        storage.emplaceAt(4, 3, 'x');

        expect(storage.get(4) == "xxx");
    };
}

void test_emplace_at_returns_reference_to_stored_component() {
    "emplaceAt returns the stored reference"_test = [] {
        ECS::ComponentStorage<Health> storage;
        auto& stored = storage.emplaceAt(4, 7);
        stored.value = 8;

        expect(storage.get(4).value == 8);
    };
}

void test_emplace_at_supports_sparse_entity_ids() {
    "emplaceAt supports sparse ids"_test = [] {
        ECS::ComponentStorage<Health> storage;
        storage.emplaceAt(80, 7);

        expect(storage.has(80));
        expect(!storage.has(79));
    };
}

void test_emplace_at_replaces_existing_component() {
    "emplaceAt replaces an existing component"_test = [] {
        ECS::ComponentStorage<Health> storage;
        storage.emplaceAt(4, 1);
        storage.emplaceAt(4, 2);

        expect(storage.size() == std::size_t{1});
        expect(storage.get(4).value == 2);
    };
}

void test_emplace_at_replacement_does_not_increase_size() {
    "emplaceAt replacement keeps the size"_test = [] {
        ECS::ComponentStorage<Health> storage;
        storage.emplaceAt(4, 1);
        storage.emplaceAt(4, 2);

        expect(storage.size() == std::size_t{1});
    };
}

void test_emplace_at_replacement_keeps_entity_presence() {
    "emplaceAt replacement keeps entity presence"_test = [] {
        ECS::ComponentStorage<Health> storage;
        storage.emplaceAt(4, 1);
        storage.emplaceAt(4, 2);

        expect(storage.has(4));
    };
}

void test_emplace_at_supports_non_copyable_components() {
    "emplaceAt supports non-copyable components"_test = [] {
        ECS::ComponentStorage<MoveOnly> storage;
        storage.emplaceAt(4, 7);

        expect(storage.get(4).value == 7);
    };
}

void test_has_returns_true_for_inserted_component() {
    "has is true for an inserted component"_test = [] {
        ECS::ComponentStorage<Health> storage;
        storage.emplaceAt(1, 1);

        expect(storage.has(1));
    };
}

void test_has_returns_false_for_unknown_entity() {
    "has is false for an unknown entity"_test = [] {
        ECS::ComponentStorage<Health> storage;
        storage.emplaceAt(1, 1);

        expect(!storage.has(2));
    };
}

void test_has_returns_false_for_entity_without_component() {
    "has is false for an entity without a component"_test = [] {
        ECS::ComponentStorage<Health> storage;
        storage.emplaceAt(1, 1);

        expect(!storage.has(0));
    };
}

void test_has_returns_false_for_out_of_range_entity_id() {
    "has is false for an out-of-range entity id"_test = [] {
        const ECS::ComponentStorage<Health> storage;

        expect(!storage.has(100));
    };
}

void test_get_returns_mutable_component() {
    "get returns a mutable component"_test = [] {
        ECS::ComponentStorage<Health> storage;
        storage.emplaceAt(1, 1);
        storage.get(1).value = 2;

        expect(storage.get(1).value == 2);
    };
}

void test_get_const_returns_read_only_component() {
    "get returns a const component from const storage"_test = [] {
        ECS::ComponentStorage<Health> storage;
        storage.emplaceAt(1, 7);
        const auto& constStorage = storage;

        expect(constStorage.get(1).value == 7);
    };
}

void test_get_throws_for_missing_component() {
    "get throws for a missing component"_test = [] {
        ECS::ComponentStorage<Health> storage;

        expect(throws<std::out_of_range>([&] {
            storage.get(1);
        }));
    };
}

void test_get_throws_for_out_of_range_entity_id() {
    "get throws for an out-of-range entity id"_test = [] {
        ECS::ComponentStorage<Health> storage;

        expect(throws<std::out_of_range>([&] {
            storage.get(100);
        }));
    };
}

void test_entity_at_returns_entity_id() {
    "entityAt returns the stored entity id"_test = [] {
        ECS::ComponentStorage<Health> storage;
        storage.emplaceAt(42, 7);

        expect(storage.entityAt(0) == std::size_t{42});
    };
}

void test_entity_at_returns_dense_entity_order() {
    "entityAt follows dense insertion order"_test = [] {
        ECS::ComponentStorage<Health> storage;
        storage.emplaceAt(4, 4);
        storage.emplaceAt(2, 2);

        expect(storage.entityAt(0) == std::size_t{4});
        expect(storage.entityAt(1) == std::size_t{2});
    };
}

void test_entity_at_throws_for_out_of_range_index() {
    "entityAt throws for an invalid packed index"_test = [] {
        ECS::ComponentStorage<Health> storage;

        expect(throws<std::out_of_range>([&] {
            static_cast<void>(storage.entityAt(0));
        }));
    };
}

void test_subscript_returns_component_by_dense_index() {
    "operator subscript returns a dense component"_test = [] {
        ECS::ComponentStorage<Health> storage;
        storage.emplaceAt(4, 7);

        expect(storage[0].value == 7);
    };
}

void test_const_subscript_returns_component_by_dense_index() {
    "const operator subscript returns a dense component"_test = [] {
        ECS::ComponentStorage<Health> storage;
        storage.emplaceAt(4, 7);
        const auto& constStorage = storage;

        expect(constStorage[0].value == 7);
    };
}

void test_subscript_reference_allows_mutation() {
    "operator subscript allows mutation"_test = [] {
        ECS::ComponentStorage<Health> storage;
        storage.emplaceAt(4, 7);
        storage[0].value = 8;

        expect(storage.get(4).value == 8);
    };
}

void test_begin_and_end_are_equal_for_empty_storage() {
    "begin and end are equal for empty storage"_test = [] {
        const ECS::ComponentStorage<Health> storage;

        expect(storage.begin() == storage.end());
    };
}

void test_iteration_visits_all_components() {
    "iteration visits all components"_test = [] {
        ECS::ComponentStorage<Health> storage;
        storage.emplaceAt(1, 1);
        storage.emplaceAt(2, 2);
        storage.emplaceAt(3, 3);

        expect(values(storage) == std::vector<int>({1, 2, 3}));
    };
}

void test_iteration_preserves_dense_insertion_order() {
    "iteration preserves dense insertion order"_test = [] {
        ECS::ComponentStorage<Health> storage;
        storage.emplaceAt(8, 8);
        storage.emplaceAt(2, 2);

        expect(values(storage) == std::vector<int>({8, 2}));
    };
}

void test_const_iteration_visits_all_components() {
    "const iteration visits all components"_test = [] {
        ECS::ComponentStorage<Health> storage;
        storage.emplaceAt(1, 4);
        storage.emplaceAt(2, 5);
        const auto& constStorage = storage;
        int total = 0;

        for (const auto& component : constStorage) {
            total += component.value;
        }

        expect(total == 9);
    };
}

void test_iteration_allows_component_mutation() {
    "iteration allows component mutation"_test = [] {
        ECS::ComponentStorage<Health> storage;
        storage.emplaceAt(1, 1);
        storage.emplaceAt(2, 2);

        for (auto& component : storage) {
            ++component.value;
        }

        expect(storage.get(1).value == 2);
        expect(storage.get(2).value == 3);
    };
}

void test_erase_removes_existing_component() {
    "erase removes an existing component"_test = [] {
        ECS::ComponentStorage<Health> storage;
        storage.emplaceAt(1, 7);
        storage.erase(1);

        expect(storage.size() == std::size_t{0});
        expect(!storage.has(1));
    };
}

void test_erase_missing_component_is_noop() {
    "erase is a no-op for a missing component"_test = [] {
        ECS::ComponentStorage<Health> storage;
        storage.emplaceAt(1, 7);
        storage.erase(2);

        expect(storage.size() == std::size_t{1});
        expect(storage.has(1));
    };
}

void test_erase_out_of_range_entity_id_is_noop() {
    "erase is a no-op for an out-of-range entity id"_test = [] {
        ECS::ComponentStorage<Health> storage;
        storage.erase(100);

        expect(storage.size() == std::size_t{0});
    };
}

void test_erase_last_component() {
    "erase removes the last component"_test = [] {
        ECS::ComponentStorage<Health> storage;
        storage.emplaceAt(1, 7);
        storage.emplaceAt(2, 8);
        storage.erase(2);

        expect(storage.size() == std::size_t{1});
        expect(storage.has(1));
        expect(!storage.has(2));
    };
}

void test_erase_first_component_uses_swap_and_pop() {
    "erase uses swap and pop for the first component"_test = [] {
        ECS::ComponentStorage<Health> storage;
        storage.emplaceAt(1, 1);
        storage.emplaceAt(2, 2);
        storage.emplaceAt(3, 3);
        storage.erase(1);

        expect(storage.size() == std::size_t{2});
        expect(storage.entityAt(0) == std::size_t{3});
    };
}

void test_erase_middle_component_uses_swap_and_pop() {
    "erase uses swap and pop for a middle component"_test = [] {
        ECS::ComponentStorage<Health> storage;
        storage.emplaceAt(1, 1);
        storage.emplaceAt(2, 2);
        storage.emplaceAt(3, 3);
        storage.erase(2);

        expect(storage.entityAt(1) == std::size_t{3});
    };
}

void test_erase_updates_moved_entity_lookup() {
    "erase updates the moved entity lookup"_test = [] {
        ECS::ComponentStorage<Health> storage;
        storage.emplaceAt(1, 1);
        storage.emplaceAt(2, 2);
        storage.erase(1);

        expect(storage.has(2));
        expect(storage.get(2).value == 2);
        expect(storage.entityAt(0) == std::size_t{2});
    };
}

void test_erase_invalidates_only_removed_entity() {
    "erase invalidates only the removed entity"_test = [] {
        ECS::ComponentStorage<Health> storage;
        storage.emplaceAt(1, 1);
        storage.emplaceAt(2, 2);
        storage.erase(1);

        expect(!storage.has(1));
        expect(storage.has(2));
    };
}

void test_erase_then_reinsert_same_entity() {
    "erase then reinsert restores the entity"_test = [] {
        ECS::ComponentStorage<Health> storage;
        storage.emplaceAt(1, 1);
        storage.erase(1);
        storage.emplaceAt(1, 2);

        expect(storage.size() == std::size_t{1});
        expect(storage.get(1).value == 2);
    };
}

void test_erase_then_reinsert_does_not_leave_stale_lookup() {
    "erase then reinsert has no stale lookup"_test = [] {
        ECS::ComponentStorage<Health> storage;
        storage.emplaceAt(1, 1);
        storage.emplaceAt(2, 2);
        storage.erase(1);
        storage.emplaceAt(1, 3);

        expect(storage.size() == std::size_t{2});
        expect(storage.get(1).value == 3);
        expect(storage.get(2).value == 2);
    };
}

void test_repeated_erase_is_safe() {
    "repeated erase is safe"_test = [] {
        ECS::ComponentStorage<Health> storage;
        storage.emplaceAt(1, 1);
        storage.erase(1);
        storage.erase(1);

        expect(storage.size() == std::size_t{0});
    };
}

void test_storage_uses_custom_allocator() {
    "ComponentStorage supports a custom allocator"_test = [] {
        ECS::ComponentStorage<Health, TestAllocator<Health>> storage;
        storage.emplaceAt(1, 7);

        expect(storage.get(1).value == 7);
    };
}

void test_storage_supports_empty_component_type() {
    "ComponentStorage supports empty components"_test = [] {
        ECS::ComponentStorage<Empty> storage;
        storage.emplaceAt(1);

        expect(storage.size() == std::size_t{1});
        expect(storage.has(1));
    };
}

void test_storage_supports_repeated_empty_component_insertion() {
    "ComponentStorage replaces repeated empty components"_test = [] {
        ECS::ComponentStorage<Empty> storage;
        storage.emplaceAt(1);
        storage.emplaceAt(1);

        expect(storage.size() == std::size_t{1});
    };
}

void test_storage_supports_large_entity_ids() {
    "ComponentStorage supports large entity ids"_test = [] {
        ECS::ComponentStorage<Health> storage;
        constexpr std::size_t entityId = 10000;
        storage.emplaceAt(entityId, 7);

        expect(storage.has(entityId));
        expect(storage.get(entityId).value == 7);
    };
}

}

void run_component_storage_tests() {
    test_storage_is_empty_by_default();
    test_storage_has_no_component_by_default();
    test_storage_npos_is_maximum_size();
    test_reserve_does_not_change_size();
    test_reserve_supports_later_insertion();
    test_multiple_reserve_calls_preserve_components();
    test_reserve_entity_capacity_supports_sparse_ids();
    test_insert_at_copy_adds_component();
    test_insert_at_copy_returns_reference_to_stored_component();
    test_insert_at_copy_copies_value();
    test_insert_at_copy_supports_sparse_entity_ids();
    test_insert_at_copy_replaces_existing_component();
    test_insert_at_copy_replacement_does_not_increase_size();
    test_insert_at_move_adds_component();
    test_insert_at_move_stores_the_moved_value();
    test_insert_at_move_replaces_existing_component();
    test_insert_at_move_replacement_does_not_increase_size();
    test_insert_at_move_supports_move_only_components();
    test_emplace_at_constructs_component();
    test_emplace_at_forwards_constructor_arguments();
    test_emplace_at_returns_reference_to_stored_component();
    test_emplace_at_supports_sparse_entity_ids();
    test_emplace_at_replaces_existing_component();
    test_emplace_at_replacement_does_not_increase_size();
    test_emplace_at_replacement_keeps_entity_presence();
    test_emplace_at_supports_non_copyable_components();
    test_has_returns_true_for_inserted_component();
    test_has_returns_false_for_unknown_entity();
    test_has_returns_false_for_entity_without_component();
    test_has_returns_false_for_out_of_range_entity_id();
    test_get_returns_mutable_component();
    test_get_const_returns_read_only_component();
    test_get_throws_for_missing_component();
    test_get_throws_for_out_of_range_entity_id();
    test_entity_at_returns_entity_id();
    test_entity_at_returns_dense_entity_order();
    test_entity_at_throws_for_out_of_range_index();
    test_subscript_returns_component_by_dense_index();
    test_const_subscript_returns_component_by_dense_index();
    test_subscript_reference_allows_mutation();
    test_begin_and_end_are_equal_for_empty_storage();
    test_iteration_visits_all_components();
    test_iteration_preserves_dense_insertion_order();
    test_const_iteration_visits_all_components();
    test_iteration_allows_component_mutation();
    test_erase_removes_existing_component();
    test_erase_missing_component_is_noop();
    test_erase_out_of_range_entity_id_is_noop();
    test_erase_last_component();
    test_erase_first_component_uses_swap_and_pop();
    test_erase_middle_component_uses_swap_and_pop();
    test_erase_updates_moved_entity_lookup();
    test_erase_invalidates_only_removed_entity();
    test_erase_then_reinsert_same_entity();
    test_erase_then_reinsert_does_not_leave_stale_lookup();
    test_repeated_erase_is_safe();
    test_storage_uses_custom_allocator();
    test_storage_supports_empty_component_type();
    test_storage_supports_repeated_empty_component_insertion();
    test_storage_supports_large_entity_ids();
}
