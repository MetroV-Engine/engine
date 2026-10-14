#include <boost/ut.hpp>

#include <memory>
#include <string>

#include "ECS/Core/Component/ComponentOps.hpp"
#include "ECS/Core/Component/ComponentPool.hpp"

using namespace boost::ut;

namespace {
    struct OpsPosition {
        int x = 7;
        int y = 8;
    };

    struct OpsNoDefault {
        explicit OpsNoDefault(int v) : value(v) {}
        int value;
    };

    struct OpsCounted {
        static inline int alive = 0;
        OpsCounted() { ++alive; }
        OpsCounted(const OpsCounted&) { ++alive; }
        OpsCounted(OpsCounted&&) noexcept { ++alive; }
        OpsCounted& operator=(const OpsCounted&) = default;
        OpsCounted& operator=(OpsCounted&&) noexcept = default;
        ~OpsCounted() { --alive; }
    };
}

namespace {
void test_ops_make_pool_creates_an_empty_typed_pool() {
    "ComponentOps::makePool creates an empty pool of the type"_test = [] {
        const ECS::ComponentOps ops = ECS::ComponentOps::of<OpsPosition>();
        std::unique_ptr<ECS::IComponentPool> pool = ops.makePool();

        auto* typed = dynamic_cast<ECS::ComponentPool<OpsPosition>*>(pool.get());
        expect(typed != nullptr);
        expect(typed->storage().size() == 0_ul);
    };
}

void test_ops_get_returns_the_component_address() {
    "ComponentOps::get returns the address of an entity's component"_test = [] {
        const ECS::ComponentOps ops = ECS::ComponentOps::of<OpsPosition>();
        ECS::ComponentPool<OpsPosition> pool;
        OpsPosition& stored = pool.storage().insertAt(3, OpsPosition{1, 2});

        expect(ops.get(pool, 3) == static_cast<void*>(&stored));
    };
}

void test_ops_get_returns_null_when_absent() {
    "ComponentOps::get returns nullptr when the entity has no component"_test = [] {
        const ECS::ComponentOps ops = ECS::ComponentOps::of<OpsPosition>();
        ECS::ComponentPool<OpsPosition> pool;
        pool.storage().insertAt(0, OpsPosition{});

        expect(ops.get(pool, 1) == nullptr);
        expect(ops.get(pool, 100000) == nullptr);
    };
}

void test_ops_new_default_constructs_a_default_value() {
    "ComponentOps::newDefault allocates a default-constructed value"_test = [] {
        const ECS::ComponentOps ops = ECS::ComponentOps::of<OpsPosition>();
        void* raw = ops.newDefault();

        expect(raw != nullptr);
        expect(static_cast<OpsPosition*>(raw)->x == 7_i);
        expect(static_cast<OpsPosition*>(raw)->y == 8_i);
        ops.deleteValue(raw);
    };
}

void test_ops_new_default_is_null_without_default_constructor() {
    "ComponentOps::newDefault returns nullptr without a default constructor"_test = [] {
        const ECS::ComponentOps ops = ECS::ComponentOps::of<OpsNoDefault>();

        expect(ops.newDefault() == nullptr);
    };
}

void test_ops_delete_value_destroys_the_value() {
    "ComponentOps::deleteValue destroys a value allocated by newDefault"_test = [] {
        const ECS::ComponentOps ops = ECS::ComponentOps::of<OpsCounted>();
        const int before = OpsCounted::alive;
        void* raw = ops.newDefault();
        expect(OpsCounted::alive == before + 1);

        ops.deleteValue(raw);

        expect(OpsCounted::alive == before);
    };
}

void test_ops_insert_moved_inserts_a_new_component() {
    "ComponentOps::insertMoved moves a value into an empty slot"_test = [] {
        const ECS::ComponentOps ops = ECS::ComponentOps::of<OpsPosition>();
        ECS::ComponentPool<OpsPosition> pool;
        OpsPosition value{4, 5};

        void* stored = ops.insertMoved(pool, 2, &value);

        expect(pool.has(2));
        expect(stored == static_cast<void*>(&pool.storage().get(2)));
        expect(pool.storage().get(2).x == 4_i);
    };
}

void test_ops_insert_moved_replaces_an_existing_component() {
    "ComponentOps::insertMoved replaces an existing component"_test = [] {
        const ECS::ComponentOps ops = ECS::ComponentOps::of<OpsPosition>();
        ECS::ComponentPool<OpsPosition> pool;
        pool.storage().insertAt(2, OpsPosition{1, 1});
        OpsPosition value{9, 9};

        ops.insertMoved(pool, 2, &value);

        expect(pool.storage().size() == 1_ul);
        expect(pool.storage().get(2).x == 9_i);
    };
}

void test_ops_of_different_types_are_distinct() {
    "ComponentOps of two types create pools of their own type"_test = [] {
        auto first = ECS::ComponentOps::of<OpsPosition>().makePool();
        auto second = ECS::ComponentOps::of<OpsNoDefault>().makePool();

        expect(dynamic_cast<ECS::ComponentPool<OpsPosition>*>(first.get()) != nullptr);
        expect(dynamic_cast<ECS::ComponentPool<OpsPosition>*>(second.get()) == nullptr);
    };
}
}

void run_component_ops_tests() {
    test_ops_make_pool_creates_an_empty_typed_pool();
    test_ops_get_returns_the_component_address();
    test_ops_get_returns_null_when_absent();
    test_ops_new_default_constructs_a_default_value();
    test_ops_new_default_is_null_without_default_constructor();
    test_ops_delete_value_destroys_the_value();
    test_ops_insert_moved_inserts_a_new_component();
    test_ops_insert_moved_replaces_an_existing_component();
    test_ops_of_different_types_are_distinct();
}
