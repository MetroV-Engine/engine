#include <boost/ut.hpp>

#include <stdexcept>
#include <string>
#include <utility>

#include "ECS/Core/Resource/ResourceStore.hpp"

using namespace boost::ut;

namespace {
    struct Clock {
        double elapsed{0.0};
    };

    struct Settings {
        std::string name;
        int volume{0};

        Settings() = default;
        Settings(std::string n, int v) : name(std::move(n)), volume(v) {}
    };

    // Each test that needs an empty slot below a set one uses its own types,
    // so that their resourceId() order is fixed by that test.
    struct NeverSet {};
    struct SetAfterNeverSet {};
    struct ConstNeverSet {};
    struct HasNeverSet {};
    struct HasSetAfterNeverSet {};
}

namespace {
void test_set_returns_reference_to_stored_value() {
    "set returns a reference to the stored value"_test = [] {
        ECS::ResourceStore store;
        Clock& clock = store.set<Clock>(Clock{1.5});

        expect(clock.elapsed == 1.5_d);
        expect(&clock == &store.get<Clock>());
    };
}

void test_set_forwards_constructor_arguments() {
    "set forwards its arguments to the resource constructor"_test = [] {
        ECS::ResourceStore store;
        store.set<Settings>("Metro", 7);

        expect(store.get<Settings>().name == "Metro");
        expect(store.get<Settings>().volume == 7_i);
    };
}

void test_set_without_arguments_default_constructs() {
    "set without arguments default-constructs the resource"_test = [] {
        ECS::ResourceStore store;
        store.set<Clock>();

        expect(store.get<Clock>().elapsed == 0.0_d);
    };
}

void test_set_replaces_previous_value() {
    "set replaces a previously set value"_test = [] {
        ECS::ResourceStore store;
        store.set<Clock>(Clock{1.0});
        store.set<Clock>(Clock{2.0});

        expect(store.get<Clock>().elapsed == 2.0_d);
    };
}

void test_set_stores_a_non_owning_pointer() {
    "set stores a raw pointer as a non-owning reference"_test = [] {
        ECS::ResourceStore store;
        Clock external{4.0};
        store.set<Clock*>(&external);

        store.get<Clock*>()->elapsed = 8.0;

        expect(external.elapsed == 8.0_d);
        expect(!store.has<Clock>());
    };
}

void test_get_returns_mutable_reference() {
    "get returns a mutable reference"_test = [] {
        ECS::ResourceStore store;
        store.set<Clock>(Clock{0.0});

        store.get<Clock>().elapsed = 42.0;

        expect(store.get<Clock>().elapsed == 42.0_d);
    };
}

void test_get_on_unset_type_throws() {
    "get on a type that was never set throws"_test = [] {
        ECS::ResourceStore store;

        expect(throws<std::out_of_range>([&] { (void)store.get<Clock>(); }));
    };
}

void test_get_on_empty_slot_below_a_set_one_throws() {
    "get throws for an unset type whose slot exists"_test = [] {
        ECS::ResourceStore store;
        (void)store.has<NeverSet>();          // allocates NeverSet's id first
        store.set<SetAfterNeverSet>();        // widens the table past it

        expect(throws<std::out_of_range>([&] { (void)store.get<NeverSet>(); }));
    };
}

void test_const_get_returns_value() {
    "get is available on a const store"_test = [] {
        ECS::ResourceStore store;
        store.set<Clock>(Clock{5.0});
        const ECS::ResourceStore& constStore = store;

        expect(constStore.get<Clock>().elapsed == 5.0_d);
    };
}

void test_const_get_on_unset_type_throws() {
    "get on a const store throws for an unset type"_test = [] {
        ECS::ResourceStore store;
        (void)store.has<ConstNeverSet>();
        store.set<Clock>();
        const ECS::ResourceStore& constStore = store;

        expect(throws<std::out_of_range>([&] { (void)constStore.get<ConstNeverSet>(); }));
        expect(throws<std::out_of_range>([&] { (void)constStore.get<Settings>(); }));
    };
}

void test_has_is_false_before_set() {
    "has is false before the resource is set"_test = [] {
        const ECS::ResourceStore store;

        expect(!store.has<Clock>());
    };
}

void test_has_is_true_after_set() {
    "has is true after the resource is set"_test = [] {
        ECS::ResourceStore store;
        store.set<Clock>();

        expect(store.has<Clock>());
    };
}

void test_has_is_false_for_empty_slot_below_a_set_one() {
    "has is false for an unset type whose slot exists"_test = [] {
        ECS::ResourceStore store;
        (void)store.has<HasNeverSet>();       // allocates HasNeverSet's id first
        store.set<HasSetAfterNeverSet>();     // widens the table past it

        expect(!store.has<HasNeverSet>());
    };
}

void test_types_are_stored_independently() {
    "a store keeps each resource type separately"_test = [] {
        ECS::ResourceStore store;
        store.set<Clock>(Clock{3.0});
        store.set<Settings>("World", 1);

        expect(store.get<Clock>().elapsed == 3.0_d);
        expect(store.get<Settings>().name == "World");
    };
}

void test_stores_are_independent() {
    "two stores do not share their resources"_test = [] {
        ECS::ResourceStore first;
        ECS::ResourceStore second;
        first.set<Clock>(Clock{1.0});

        expect(first.has<Clock>());
        expect(!second.has<Clock>());
    };
}
}

void run_resource_store_tests() {
    test_set_returns_reference_to_stored_value();
    test_set_forwards_constructor_arguments();
    test_set_without_arguments_default_constructs();
    test_set_replaces_previous_value();
    test_set_stores_a_non_owning_pointer();
    test_get_returns_mutable_reference();
    test_get_on_unset_type_throws();
    test_get_on_empty_slot_below_a_set_one_throws();
    test_const_get_returns_value();
    test_const_get_on_unset_type_throws();
    test_has_is_false_before_set();
    test_has_is_true_after_set();
    test_has_is_false_for_empty_slot_below_a_set_one();
    test_types_are_stored_independently();
    test_stores_are_independent();
}
