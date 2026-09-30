#include <boost/ut.hpp>

#include <stdexcept>
#include <string>

#include "ECS/Registry.hpp"

using namespace boost::ut;

namespace {
    struct Clock {
        double elapsed{0.0};
    };

    struct Settings {
        std::string name;
    };

    struct Position {
        int x;
        int y;
    };
}

namespace {
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

}

void run_resources_tests() {
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
}
