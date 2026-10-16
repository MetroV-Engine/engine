#include <boost/ut.hpp>

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <type_traits>

#include "ECS/Core/Component/ComponentPool.hpp"

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
        auto& typed = static_cast<ECS::ComponentPool<Position>&>(*pool);
        typed.storage().emplaceAt(0, 1, 2);

        pool->erase(0);

        expect(!typed.storage().has(0));
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

        expect(pool.storage().has(4));
        pool.erase(4);
        expect(!pool.storage().has(4));
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
        expect(pool.storage().has(1));
    };
}

void test_component_pool_erase_updates_swap_and_pop_lookup() {
    "ComponentPool erase preserves moved lookup"_test = [] {
        ECS::ComponentPool<Position> pool;
        pool.storage().emplaceAt(1, 1, 1);
        pool.storage().emplaceAt(2, 2, 2);
        pool.erase(1);

        expect(pool.storage().has(2));
        expect(pool.storage().get(2).x == 2);
    };
}

void test_component_pool_can_be_used_through_interface() {
    "ComponentPool supports type-erased access"_test = [] {
        ECS::ComponentPool<Position> concrete;
        concrete.storage().emplaceAt(7, 4, 5);
        ECS::IComponentPool& pool = concrete;

        expect(concrete.storage().has(7));
        pool.erase(7);
        expect(!concrete.storage().has(7));
    };
}

void test_different_component_pools_have_independent_storage() {
    "different ComponentPool types have independent storage"_test = [] {
        ECS::ComponentPool<Position> positions;
        ECS::ComponentPool<Health> health;
        positions.storage().emplaceAt(1, 2, 3);
        health.storage().emplaceAt(1, 100);

        expect(positions.storage().has(1));
        expect(health.storage().has(1));
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

void test_component_pool_on_remove_fires_once_on_erase() {
    "ComponentPool onRemove fires once when a component is erased"_test = [] {
        ECS::ComponentPool<Position> pool;
        pool.storage().emplaceAt(1, 2, 3);
        int callCount = 0;
        pool.setOnRemove([&callCount](Position&) { ++callCount; });

        pool.erase(1);

        expect(callCount == 1);
    };
}

void test_component_pool_on_remove_receives_the_exact_instance() {
    "ComponentPool onRemove receives the exact instance being erased"_test = [] {
        ECS::ComponentPool<Position> pool;
        pool.storage().emplaceAt(1, 10, 20);
        Position seen{0, 0};
        pool.setOnRemove([&seen](Position& position) { seen = position; });

        pool.erase(1);

        expect(seen.x == 10);
        expect(seen.y == 20);
    };
}

void test_component_pool_on_remove_does_not_fire_for_missing_entity() {
    "ComponentPool onRemove does not fire when the entity has no component"_test = [] {
        ECS::ComponentPool<Position> pool;
        int callCount = 0;
        pool.setOnRemove([&callCount](Position&) { ++callCount; });

        pool.erase(99);

        expect(callCount == 0);
    };
}

void test_component_pool_on_remove_throwing_callback_is_contained() {
    "ComponentPool onRemove exceptions never escape erase"_test = [] {
        ECS::ComponentPool<Position> pool;
        pool.storage().emplaceAt(1, 2, 3);
        pool.setOnRemove([](Position&) { throw std::runtime_error("boom"); });

        pool.erase(1);

        expect(!pool.storage().has(1));
    };
}

void test_component_pool_without_hook_is_unaffected() {
    "ComponentPool without a hook still erases normally"_test = [] {
        ECS::ComponentPool<Position> pool;
        pool.storage().emplaceAt(1, 2, 3);

        pool.erase(1);

        expect(!pool.storage().has(1));
    };
}

void test_component_pool_set_on_remove_replaces_previous_callback() {
    "ComponentPool setOnRemove replaces the previous callback"_test = [] {
        ECS::ComponentPool<Position> pool;
        pool.storage().emplaceAt(1, 2, 3);
        int firstCalls = 0;
        int secondCalls = 0;
        pool.setOnRemove([&firstCalls](Position&) { ++firstCalls; });
        pool.setOnRemove([&secondCalls](Position&) { ++secondCalls; });

        pool.erase(1);

        expect(firstCalls == 0);
        expect(secondCalls == 1);
    };
}

void test_make_pool_creates_an_empty_typed_pool() {
    "makePool creates an empty pool of its component type"_test = [] {
        const ECS::PoolFactory factory = &ECS::makePool<Position>;
        std::unique_ptr<ECS::IComponentPool> pool = factory();

        auto* typed = dynamic_cast<ECS::ComponentPool<Position>*>(pool.get());
        expect((typed != nullptr) >> fatal);
        expect(typed->storage().size() == std::size_t{0});
        expect(dynamic_cast<ECS::ComponentPool<Health>*>(pool.get()) == nullptr);
    };
}

void test_make_pool_returns_a_new_pool_each_call() {
    "makePool returns a distinct pool on each call"_test = [] {
        auto first = ECS::makePool<Position>();
        auto second = ECS::makePool<Position>();

        expect(first.get() != second.get());
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
    test_component_pool_erase_removes_existing_component();
    test_component_pool_erase_missing_component_is_noop();
    test_component_pool_erase_out_of_range_entity_is_noop();
    test_component_pool_erase_updates_swap_and_pop_lookup();
    test_component_pool_can_be_used_through_interface();
    test_different_component_pools_have_independent_storage();
    test_component_pool_does_not_expose_wrong_component_type();
    test_component_pool_on_remove_fires_once_on_erase();
    test_component_pool_on_remove_receives_the_exact_instance();
    test_component_pool_on_remove_does_not_fire_for_missing_entity();
    test_component_pool_on_remove_throwing_callback_is_contained();
    test_component_pool_without_hook_is_unaffected();
    test_component_pool_set_on_remove_replaces_previous_callback();
    test_make_pool_creates_an_empty_typed_pool();
    test_make_pool_returns_a_new_pool_each_call();
}
