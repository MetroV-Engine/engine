#include <boost/ut.hpp>

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <vector>

#include "ECS/Core/Component/ComponentId.hpp"
#include "ECS/Core/Component/ComponentOps.hpp"
#include "ECS/Core/Component/ComponentPool.hpp"
#include "ECS/Core/Component/ComponentStore.hpp"

using namespace boost::ut;

namespace {
    struct StorePosition { int x = 0; int y = 0; };
    struct StoreVelocity { int dx = 0; };
    struct StoreNeverAdded { int value = 0; };
    struct StoreById { int value = 3; };
    struct StoreUnregistered { int value = 0; };

    template<typename T>
    ECS::ComponentId idOf() { return ECS::componentId<T>(); }

    std::unique_ptr<ECS::IComponentPool> makeByIdPool() {
        return std::make_unique<ECS::ComponentPool<StoreById>>();
    }
}

namespace {
// --- ensure / storage / find ------------------------------------------------

void test_store_ensure_registers_once() {
    "ComponentStore::ensure registers the type once and returns the same storage"_test = [] {
        ECS::ComponentStore store;
        auto& first = store.ensure<StorePosition>();
        auto& second = store.ensure<StorePosition>();

        expect(&first == &second);
        expect(store.hasPool(idOf<StorePosition>()));
    };
}

void test_store_storage_returns_registered_storage() {
    "ComponentStore::storage returns the storage of a registered type"_test = [] {
        ECS::ComponentStore store;
        auto& ensured = store.ensure<StorePosition>();

        expect(&store.storage<StorePosition>() == &ensured);
    };
}

void test_store_storage_throws_when_unregistered() {
    "ComponentStore::storage throws for a type that was never registered"_test = [] {
        ECS::ComponentStore store;

        expect(throws<std::out_of_range>([&] { (void)store.storage<StoreNeverAdded>(); }));
    };
}

void test_store_const_storage() {
    "ComponentStore::storage on a const store returns or throws"_test = [] {
        ECS::ComponentStore store;
        store.insert(0, StorePosition{1, 2});
        const ECS::ComponentStore& constStore = store;

        expect(constStore.storage<StorePosition>().get(0).x == 1_i);
        expect(throws<std::out_of_range>([&] { (void)constStore.storage<StoreNeverAdded>(); }));
    };
}

void test_store_find_returns_storage_or_null() {
    "ComponentStore::find returns the storage when registered, nullptr otherwise"_test = [] {
        ECS::ComponentStore store;
        expect(store.find<StorePosition>() == nullptr);

        auto& ensured = store.ensure<StorePosition>();

        expect(store.find<StorePosition>() == &ensured);
        expect(store.find<StoreNeverAdded>() == nullptr);
    };
}

// --- insert / emplace ---------------------------------------------------------

void test_store_insert_adds_and_marks_owned() {
    "ComponentStore::insert stores the value and marks the entity as owner"_test = [] {
        ECS::ComponentStore store;
        store.addEntity(2);
        StorePosition& stored = store.insert(2, StorePosition{3, 4});

        expect(stored.x == 3_i);
        expect(store.has(2, {idOf<StorePosition>()}));
    };
}

void test_store_insert_registers_the_type() {
    "ComponentStore::insert registers the type when needed"_test = [] {
        ECS::ComponentStore store;
        store.insert(0, StoreVelocity{1});

        expect(store.find<StoreVelocity>() != nullptr);
    };
}

void test_store_insert_replaces() {
    "ComponentStore::insert replaces an existing component"_test = [] {
        ECS::ComponentStore store;
        store.insert(1, StorePosition{1, 1});
        store.insert(1, StorePosition{5, 5});

        expect(store.storage<StorePosition>().size() == 1_ul);
        expect(store.storage<StorePosition>().get(1).x == 5_i);
    };
}

void test_store_insert_without_added_entity() {
    "ComponentStore::insert works for an entity that was never added"_test = [] {
        ECS::ComponentStore store;
        store.insert(40, StorePosition{2, 2});

        expect(store.has(40, {idOf<StorePosition>()}));
        expect(!store.has(39, {idOf<StorePosition>()}));
    };
}

void test_store_storage_or_empty_returns_registered_storage() {
    "ComponentStore::storageOrEmpty returns the storage of a registered type"_test = [] {
        ECS::ComponentStore store;
        store.insert(0, StorePosition{1, 2});
        const ECS::ComponentStore& constStore = store;

        expect(&constStore.storageOrEmpty<StorePosition>() == &store.storage<StorePosition>());
    };
}

void test_store_storage_or_empty_is_empty_for_unregistered_type() {
    "ComponentStore::storageOrEmpty is empty for an unregistered type and does not register it"_test = [] {
        const ECS::ComponentStore store;

        expect(store.storageOrEmpty<StoreNeverAdded>().size() == 0_ul);
        expect(store.find<StoreNeverAdded>() == nullptr);
        expect(!store.hasPool(idOf<StoreNeverAdded>()));
    };
}

// --- onRemove -------------------------------------------------------------------

void test_store_on_remove_fires_on_remove_and_remove_entity() {
    "ComponentStore::onRemove fires from remove and removeEntity"_test = [] {
        ECS::ComponentStore store;
        int calls = 0;
        store.onRemove<StorePosition>([&calls](StorePosition&) { ++calls; });
        store.insert(0, StorePosition{});
        store.insert(1, StorePosition{});

        store.remove(0, idOf<StorePosition>());
        store.removeEntity(1);

        expect(calls == 2_i);
    };
}

void test_store_on_remove_registers_the_type() {
    "ComponentStore::onRemove registers the type when needed"_test = [] {
        ECS::ComponentStore store;
        store.onRemove<StoreVelocity>([](StoreVelocity&) {});

        expect(store.find<StoreVelocity>() != nullptr);
    };
}

void test_store_on_remove_does_not_fire_without_component() {
    "ComponentStore::onRemove does not fire when nothing is erased"_test = [] {
        ECS::ComponentStore store;
        int calls = 0;
        store.onRemove<StorePosition>([&calls](StorePosition&) { ++calls; });
        store.addEntity(0);

        store.remove(0, idOf<StorePosition>());
        store.removeEntity(0);

        expect(calls == 0_i);
    };
}

// --- ComponentId-based access ----------------------------------------------------

void test_store_ensure_pool_creates_once() {
    "ComponentStore::ensurePool creates the pool once"_test = [] {
        ECS::ComponentStore store;
        const ECS::ComponentId id = idOf<StoreById>();
        expect(!store.hasPool(id));

        StoreById value{5};
        store.ensurePool(id, &makeByIdPool);
        void* first = store.insertMoved(0, id, &value, ECS::ComponentOps::of<StoreById>());
        store.ensurePool(id, &makeByIdPool);   // must keep the existing pool and its component

        expect(store.hasPool(id));
        expect(store.get(0, id, ECS::ComponentOps::of<StoreById>()) == first);
    };
}

void test_store_has_pool_is_false_for_unknown_ids() {
    "ComponentStore::hasPool is false for an id without a pool"_test = [] {
        ECS::ComponentStore store;
        store.ensure<StorePosition>();

        expect(!store.hasPool(idOf<StoreUnregistered>()));
        expect(!store.hasPool(100000));
    };
}

void test_store_require_registered() {
    "ComponentStore::requireRegistered throws only for an id without a pool"_test = [] {
        ECS::ComponentStore store;
        store.ensure<StorePosition>();

        store.requireRegistered(idOf<StorePosition>());   // no throw
        expect(throws<std::out_of_range>([&] { store.requireRegistered(idOf<StoreUnregistered>()); }));
        expect(throws<std::out_of_range>([&] { store.requireRegistered(100000); }));
    };
}

void test_store_has_checks_every_id() {
    "ComponentStore::has is true only when the entity owns every id"_test = [] {
        ECS::ComponentStore store;
        store.insert(0, StorePosition{});
        store.insert(0, StoreVelocity{});
        store.insert(1, StorePosition{});

        expect(store.has(0, {idOf<StorePosition>(), idOf<StoreVelocity>()}));
        expect(!store.has(1, {idOf<StorePosition>(), idOf<StoreVelocity>()}));
    };
}

void test_store_has_edge_cases() {
    "ComponentStore::has handles empty lists, unknown ids and unknown entities"_test = [] {
        ECS::ComponentStore store;
        store.insert(0, StorePosition{});

        expect(store.has(0, {}));
        expect(!store.has(0, {idOf<StoreUnregistered>()}));
        expect(!store.has(500, {idOf<StorePosition>()}));
    };
}

void test_store_get_returns_address_or_null() {
    "ComponentStore::get returns the component address, or nullptr"_test = [] {
        ECS::ComponentStore store;
        const auto ops = ECS::ComponentOps::of<StorePosition>();
        expect(store.get(0, idOf<StorePosition>(), ops) == nullptr);

        StorePosition& stored = store.insert(0, StorePosition{1, 2});

        expect(store.get(0, idOf<StorePosition>(), ops) == static_cast<void*>(&stored));
        expect(store.get(1, idOf<StorePosition>(), ops) == nullptr);
    };
}

void test_store_insert_moved_inserts_and_marks_owned() {
    "ComponentStore::insertMoved inserts or replaces and marks the entity as owner"_test = [] {
        ECS::ComponentStore store;
        const ECS::ComponentId id = idOf<StoreById>();
        const auto ops = ECS::ComponentOps::of<StoreById>();
        store.ensurePool(id, ops.makePool);
        StoreById first{1};
        StoreById second{2};

        store.insertMoved(4, id, &first, ops);
        void* stored = store.insertMoved(4, id, &second, ops);

        expect(store.has(4, {id}));
        expect(static_cast<StoreById*>(stored)->value == 2_i);
    };
}

void test_store_insert_moved_throws_without_pool() {
    "ComponentStore::insertMoved throws when there is no pool for the id"_test = [] {
        ECS::ComponentStore store;
        StoreUnregistered value;

        expect(throws<std::out_of_range>([&] {
            (void)store.insertMoved(0, idOf<StoreUnregistered>(), &value,
                                    ECS::ComponentOps::of<StoreUnregistered>());
        }));
        expect(!store.has(0, {idOf<StoreUnregistered>()}));
    };
}

void test_store_remove_erases_one_component() {
    "ComponentStore::remove erases one component and keeps the others"_test = [] {
        ECS::ComponentStore store;
        store.insert(0, StorePosition{});
        store.insert(0, StoreVelocity{});

        store.remove(0, idOf<StorePosition>());

        expect(!store.has(0, {idOf<StorePosition>()}));
        expect(store.has(0, {idOf<StoreVelocity>()}));
        expect(!store.storage<StorePosition>().has(0));
    };
}

void test_store_remove_is_a_no_op_without_component() {
    "ComponentStore::remove does nothing without the component, the pool or the row"_test = [] {
        ECS::ComponentStore store;
        store.insert(0, StorePosition{});

        store.remove(1, idOf<StorePosition>());
        store.remove(0, idOf<StoreUnregistered>());
        store.remove(1000, idOf<StorePosition>());

        expect(store.has(0, {idOf<StorePosition>()}));
    };
}

void test_store_ids_of_lists_owned_ids_ascending() {
    "ComponentStore::idsOf lists the owned ids in ascending order"_test = [] {
        ECS::ComponentStore store;
        store.insert(0, StoreVelocity{});
        store.insert(0, StorePosition{});

        const std::vector<ECS::ComponentId> ids = store.idsOf(0);

        expect(ids.size() == 2_ul);
        expect(ids[0] < ids[1]);
    };
}

void test_store_ids_of_is_empty_without_components() {
    "ComponentStore::idsOf is empty for an entity without components or row"_test = [] {
        ECS::ComponentStore store;
        store.ensure<StorePosition>();
        store.addEntity(0);

        expect(store.idsOf(0).empty());
        expect(store.idsOf(999).empty());
    };
}

// --- Entity lifecycle ----------------------------------------------------------

void test_store_add_entity_gives_an_empty_row() {
    "ComponentStore::addEntity gives a recycled index an empty row"_test = [] {
        ECS::ComponentStore store;
        store.insert(3, StorePosition{});
        store.storage<StorePosition>().erase(3);   // pool emptied without the store

        store.addEntity(3);

        expect(store.idsOf(3).empty());
    };
}

void test_store_remove_entity_erases_everything() {
    "ComponentStore::removeEntity erases every component and empties the row"_test = [] {
        ECS::ComponentStore store;
        store.insert(0, StorePosition{});
        store.insert(0, StoreVelocity{});
        store.insert(1, StorePosition{});

        store.removeEntity(0);

        expect(store.idsOf(0).empty());
        expect(!store.storage<StorePosition>().has(0));
        expect(!store.storage<StoreVelocity>().has(0));
        expect(store.has(1, {idOf<StorePosition>()}));
    };
}

void test_store_remove_entity_without_row_is_safe() {
    "ComponentStore::removeEntity is safe for an entity that has no row"_test = [] {
        ECS::ComponentStore store;
        store.ensure<StorePosition>();

        store.removeEntity(12345);

        expect(store.idsOf(12345).empty());
    };
}

void test_stores_are_independent() {
    "two ComponentStores do not share pools or rows"_test = [] {
        ECS::ComponentStore first;
        ECS::ComponentStore second;
        first.insert(0, StorePosition{});

        expect(second.find<StorePosition>() == nullptr);
        expect(!second.has(0, {idOf<StorePosition>()}));
    };
}
}

void run_component_store_tests() {
    test_store_ensure_registers_once();
    test_store_storage_returns_registered_storage();
    test_store_storage_throws_when_unregistered();
    test_store_const_storage();
    test_store_find_returns_storage_or_null();
    test_store_insert_adds_and_marks_owned();
    test_store_insert_registers_the_type();
    test_store_insert_replaces();
    test_store_insert_without_added_entity();
    test_store_storage_or_empty_returns_registered_storage();
    test_store_storage_or_empty_is_empty_for_unregistered_type();
    test_store_on_remove_fires_on_remove_and_remove_entity();
    test_store_on_remove_registers_the_type();
    test_store_on_remove_does_not_fire_without_component();
    test_store_ensure_pool_creates_once();
    test_store_has_pool_is_false_for_unknown_ids();
    test_store_require_registered();
    test_store_has_checks_every_id();
    test_store_has_edge_cases();
    test_store_get_returns_address_or_null();
    test_store_insert_moved_inserts_and_marks_owned();
    test_store_insert_moved_throws_without_pool();
    test_store_remove_erases_one_component();
    test_store_remove_is_a_no_op_without_component();
    test_store_ids_of_lists_owned_ids_ascending();
    test_store_ids_of_is_empty_without_components();
    test_store_add_entity_gives_an_empty_row();
    test_store_remove_entity_erases_everything();
    test_store_remove_entity_without_row_is_safe();
    test_stores_are_independent();
}
