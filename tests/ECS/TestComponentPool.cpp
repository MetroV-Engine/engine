#include <boost/ut.hpp>

#include <cstddef>
#include <memory>
#include <type_traits>

#include "ECS/ComponentPool.hpp"

using namespace boost::ut;

namespace {
    struct Position {
        int x;
        int y;
    };

    struct Health {
        int value;
    };
}

namespace {
void test_component_pool_interface_has_virtual_destructor() {
    "IComponentPool has a virtual destructor"_test = [] {
        expect(std::has_virtual_destructor_v<ECS::IComponentPool>);
    };
}

void test_component_pool_is_polymorphic() {
    "IComponentPool is polymorphic"_test = [] {
        expect(std::is_polymorphic_v<ECS::IComponentPool>);
    };
}

void test_component_pool_can_be_owned_by_unique_ptr_to_interface() {
    "ComponentPool can be owned through its interface"_test = [] {
        std::unique_ptr<ECS::IComponentPool> pool =
            std::make_unique<ECS::ComponentPool<Position>>();

        expect(!pool->has(0));
    };
}

void test_component_pool_storage_is_empty_initially() {
    "ComponentPool storage is empty initially"_test = [] {
        const ECS::ComponentPool<Position> pool;

        expect(pool.storage().size() == std::size_t{0});
    };
}

void test_component_pool_storage_returns_mutable_storage() {
    "ComponentPool returns mutable typed storage"_test = [] {
        ECS::ComponentPool<Position> pool;
        pool.storage().emplaceAt(3, 10, 20);

        expect(pool.storage().get(3).x == 10);
        expect(pool.storage().get(3).y == 20);
    };
}

void test_component_pool_const_storage_returns_const_storage() {
    "ComponentPool returns const typed storage"_test = [] {
        ECS::ComponentPool<Position> pool;
        pool.storage().emplaceAt(3, 10, 20);
        const auto& constPool = pool;

        static_assert(std::is_const_v<std::remove_reference_t<decltype(constPool.storage())>>);
        expect(constPool.storage().get(3).x == 10);
    };
}

void test_component_pool_storage_is_the_same_underlying_storage() {
    "ComponentPool accessors share the underlying storage"_test = [] {
        ECS::ComponentPool<Position> pool;
        pool.storage().emplaceAt(4, 7, 8);

        expect(pool.has(4));
        pool.erase(4);
        expect(!pool.storage().has(4));
    };
}

void test_component_pool_has_returns_false_when_empty() {
    "ComponentPool has is false when empty"_test = [] {
        const ECS::ComponentPool<Position> pool;

        expect(!pool.has(0));
    };
}

void test_component_pool_has_returns_true_for_stored_component() {
    "ComponentPool has is true for a stored component"_test = [] {
        ECS::ComponentPool<Position> pool;
        pool.storage().emplaceAt(5, 1, 2);

        expect(pool.has(5));
    };
}

void test_component_pool_has_returns_false_after_erase() {
    "ComponentPool has is false after erase"_test = [] {
        ECS::ComponentPool<Position> pool;
        pool.storage().emplaceAt(5, 1, 2);
        pool.erase(5);

        expect(!pool.has(5));
    };
}

void test_component_pool_has_returns_false_for_unknown_entity() {
    "ComponentPool has is false for an unknown entity"_test = [] {
        ECS::ComponentPool<Position> pool;
        pool.storage().emplaceAt(5, 1, 2);

        expect(!pool.has(6));
    };
}

void test_component_pool_erase_removes_existing_component() {
    "ComponentPool erase removes an existing component"_test = [] {
        ECS::ComponentPool<Position> pool;
        pool.storage().emplaceAt(1, 2, 3);
        pool.erase(1);

        expect(pool.storage().size() == std::size_t{0});
    };
}

void test_component_pool_erase_missing_component_is_noop() {
    "ComponentPool erase is a no-op for a missing component"_test = [] {
        ECS::ComponentPool<Position> pool;
        pool.erase(99);

        expect(pool.storage().size() == std::size_t{0});
    };
}

void test_component_pool_erase_out_of_range_entity_is_noop() {
    "ComponentPool erase is a no-op for an unknown entity"_test = [] {
        ECS::ComponentPool<Position> pool;
        pool.storage().emplaceAt(1, 2, 3);
        pool.erase(1000);

        expect(pool.storage().size() == std::size_t{1});
        expect(pool.has(1));
    };
}

void test_component_pool_erase_updates_swap_and_pop_lookup() {
    "ComponentPool erase preserves moved lookup"_test = [] {
        ECS::ComponentPool<Position> pool;
        pool.storage().emplaceAt(1, 1, 1);
        pool.storage().emplaceAt(2, 2, 2);
        pool.erase(1);

        expect(pool.has(2));
        expect(pool.storage().get(2).x == 2);
    };
}

void test_component_pool_can_be_used_through_interface() {
    "ComponentPool supports type-erased access"_test = [] {
        ECS::ComponentPool<Position> concrete;
        concrete.storage().emplaceAt(7, 4, 5);
        ECS::IComponentPool& pool = concrete;

        expect(pool.has(7));
        pool.erase(7);
        expect(!pool.has(7));
    };
}

void test_different_component_pools_have_independent_storage() {
    "different ComponentPool types have independent storage"_test = [] {
        ECS::ComponentPool<Position> positions;
        ECS::ComponentPool<Health> health;
        positions.storage().emplaceAt(1, 2, 3);
        health.storage().emplaceAt(1, 100);

        expect(positions.has(1));
        expect(health.has(1));
        expect(positions.storage().get(1).x == 2);
        expect(health.storage().get(1).value == 100);
    };
}

void test_component_pool_does_not_expose_wrong_component_type() {
    "ComponentPool exposes its own component type"_test = [] {
        ECS::ComponentPool<Position> pool;

        static_assert(std::is_same_v<
            decltype(pool.storage()),
            ECS::ComponentStorage<Position>&
        >);
        expect(pool.storage().size() == std::size_t{0});
    };
}

}

void run_component_pool_tests() {
    test_component_pool_interface_has_virtual_destructor();
    test_component_pool_is_polymorphic();
    test_component_pool_can_be_owned_by_unique_ptr_to_interface();
    test_component_pool_storage_is_empty_initially();
    test_component_pool_storage_returns_mutable_storage();
    test_component_pool_const_storage_returns_const_storage();
    test_component_pool_storage_is_the_same_underlying_storage();
    test_component_pool_has_returns_false_when_empty();
    test_component_pool_has_returns_true_for_stored_component();
    test_component_pool_has_returns_false_after_erase();
    test_component_pool_has_returns_false_for_unknown_entity();
    test_component_pool_erase_removes_existing_component();
    test_component_pool_erase_missing_component_is_noop();
    test_component_pool_erase_out_of_range_entity_is_noop();
    test_component_pool_erase_updates_swap_and_pop_lookup();
    test_component_pool_can_be_used_through_interface();
    test_different_component_pools_have_independent_storage();
    test_component_pool_does_not_expose_wrong_component_type();
}
