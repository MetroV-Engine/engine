#include <boost/ut.hpp>

#include <stdexcept>
#include <string>
#include <type_traits>

#include "Engine/Services/Services.hpp"

using namespace boost::ut;

namespace {
    static_assert(!std::is_copy_constructible_v<Services>, "services own objects: not copyable");
    static_assert(!std::is_copy_assignable_v<Services>, "services own objects: not copyable");

    struct Counter {
        explicit Counter(int start = 0) : value(start) {}
        int value;
    };

    struct Name {
        std::string text;
    };

    struct Tracked {
        explicit Tracked(int& destroyed) : _destroyed(destroyed) {}
        ~Tracked() { ++_destroyed; }
        int& _destroyed;
    };
}

void run_services_tests() {
    // ---- set / get / has

    "a new Services is empty"_test = [] {
        Services services;
        expect(services.size() == std::size_t{0});
        expect(!services.has<Counter>());
    };

    "set constructs the service with the given arguments"_test = [] {
        Services services;
        Counter& counter = services.set<Counter>(7);
        expect(counter.value == 7);
        expect(services.has<Counter>());
        expect(services.size() == std::size_t{1});
    };

    "get returns the stored service"_test = [] {
        Services services;
        Counter& stored = services.set<Counter>(3);
        expect(&services.get<Counter>() == &stored);
        const Services& view = services;
        expect(view.get<Counter>().value == 3);
    };

    "get of a service never set throws"_test = [] {
        Services services;
        expect(throws<std::out_of_range>([&] { (void)services.get<Counter>(); }));
    };

    "set replaces a service of the same type"_test = [] {
        Services services;
        services.set<Counter>(1);
        services.set<Counter>(2);
        expect(services.get<Counter>().value == 2);
        expect(services.size() == std::size_t{1});
    };

    "services of different types are independent"_test = [] {
        Services services;
        services.set<Counter>(5);
        services.set<Name>(Name{"abc"});
        expect(services.get<Counter>().value == 5);
        expect(services.get<Name>().text == std::string("abc"));
        expect(services.size() == std::size_t{2});
    };

    // ---- injectInto

    "injectInto gives a registry a reference to every service"_test = [] {
        Services services;
        Counter& counter = services.set<Counter>(9);
        services.set<Name>(Name{"x"});
        ECS::Registry registry;

        services.injectInto(registry);

        expect(hasService<Counter>(registry));
        expect(hasService<Name>(registry));
        expect(&getService<Counter>(registry) == &counter);
    };

    "injectInto with no service leaves the registry untouched"_test = [] {
        Services services;
        ECS::Registry registry;
        services.injectInto(registry);
        expect(!hasService<Counter>(registry));
    };

    "two registries share the same service instance"_test = [] {
        Services services;
        services.set<Counter>(1);
        ECS::Registry a;
        ECS::Registry b;
        services.injectInto(a);
        services.injectInto(b);

        getService<Counter>(a).value = 42;

        expect(getService<Counter>(b).value == 42);
        expect(services.get<Counter>().value == 42);
    };

    "injectInto twice is harmless"_test = [] {
        Services services;
        services.set<Counter>(1);
        ECS::Registry registry;
        services.injectInto(registry);
        services.injectInto(registry);
        expect(&getService<Counter>(registry) == &services.get<Counter>());
    };

    "a registry injected before a replacement needs injecting again"_test = [] {
        Services services;
        services.set<Counter>(1);
        ECS::Registry registry;
        services.injectInto(registry);

        services.set<Counter>(2);
        services.injectInto(registry);

        expect(getService<Counter>(registry).value == 2);
    };

    // ---- ServiceRef / getService / hasService

    "ServiceRef gives access to the service"_test = [] {
        Counter counter(4);
        ServiceRef<Counter> ref(&counter);
        expect(&ref.get() == &counter);
        expect(&*ref == &counter);
        expect(ref->value == 4);
    };

    "getService of a service not injected throws"_test = [] {
        ECS::Registry registry;
        expect(throws<std::out_of_range>([&] { (void)getService<Counter>(registry); }));
    };

    "hasService is false on a fresh registry"_test = [] {
        ECS::Registry registry;
        expect(!hasService<Counter>(registry));
    };

    // ---- clear

    "clear destroys every service"_test = [] {
        int destroyed = 0;
        Services services;
        services.set<Tracked>(destroyed);
        services.clear();
        expect(destroyed == 1);
        expect(services.size() == std::size_t{0});
        expect(!services.has<Tracked>());
    };

    "clear on an empty Services does nothing"_test = [] {
        Services services;
        services.clear();
        expect(services.size() == std::size_t{0});
    };

    "services can be set again after clear"_test = [] {
        Services services;
        services.set<Counter>(1);
        services.clear();
        services.set<Counter>(2);
        ECS::Registry registry;
        services.injectInto(registry);
        expect(getService<Counter>(registry).value == 2);
    };

    "clear forgets what injectInto used to inject"_test = [] {
        Services services;
        services.set<Counter>(1);
        services.clear();
        ECS::Registry registry;
        services.injectInto(registry);
        expect(!hasService<Counter>(registry));
    };

    "destroying Services destroys the services"_test = [] {
        int destroyed = 0;
        {
            Services services;
            services.set<Tracked>(destroyed);
        }
        expect(destroyed == 1);
    };
}
