#include <boost/ut.hpp>

#include <stdexcept>
#include <vector>

#include "ECS/ComponentPool.hpp"
#include "ECS/Registry.hpp"
#include "RegistryTestAccess.hpp"

using namespace boost::ut;

// ComponentId-based access of the Registry: the private, type-erased core used
// by reflect::RegistryInspector (a friend of the Registry), which the typed
// has/remove API forwards to.
namespace {
    using Ids = ECS::TestAccess::RegistryIdAccess;

    struct IdPosition { int x = 7; int y = 8; };
    struct IdVelocity { int dx = 0; };
    struct IdNoDefault {
        explicit IdNoDefault(int v) : value(v) {}
        int value;
    };

    template<typename T>
    ECS::ComponentId idOf() { return ECS::componentId<T>(); }

void test_ensure_pool_creates_once() {
    "ensurePoolById creates the pool once and keeps an existing one"_test = [] {
        ECS::Registry registry;
        const auto entity = registry.spawnEntity();

        expect(!Ids::hasPoolById(registry, idOf<IdPosition>()));
        Ids::ensurePoolById(registry, idOf<IdPosition>(), ECS::ComponentOps::of<IdPosition>().makePool);
        expect(Ids::hasPoolById(registry, idOf<IdPosition>()));

        registry.addComponent(entity, IdPosition{1, 2});
        Ids::ensurePoolById(registry, idOf<IdPosition>(), ECS::ComponentOps::of<IdPosition>().makePool);
        expect(registry.getComponent<IdPosition>(entity).x == 1_i);
    };
}

void test_has_components_by_id() {
    "hasComponentsById agrees with the typed overload"_test = [] {
        ECS::Registry registry;
        const auto entity = registry.spawnEntity();
        registry.addComponent(entity, IdPosition{});

        expect(Ids::hasComponentsById(registry, entity, {idOf<IdPosition>()}));
        expect(!Ids::hasComponentsById(registry, entity, {idOf<IdPosition>(), idOf<IdVelocity>()}));
        expect(Ids::hasComponentsById(registry, entity, {}));
        expect(!Ids::hasComponentsById(registry, entity, {ECS::ComponentId{100000}}));   // never registered

        registry.killEntity(entity);
        expect(!Ids::hasComponentsById(registry, entity, {idOf<IdPosition>()}));
    };
}

void test_get_component_by_id() {
    "getComponentById returns the component address or nullptr"_test = [] {
        ECS::Registry registry;
        const auto entity = registry.spawnEntity();
        const auto other = registry.spawnEntity();
        registry.addComponent(entity, IdPosition{3, 4});

        expect(Ids::getComponentById<IdPosition>(registry, entity) == &registry.getComponent<IdPosition>(entity));
        expect(Ids::getComponentById<IdPosition>(registry, other) == nullptr);   // no component
        expect(Ids::getComponentById<IdVelocity>(registry, entity) == nullptr);  // no pool

        registry.killEntity(entity);
        expect(throws<std::invalid_argument>([&] { (void)Ids::getComponentById<IdPosition>(registry, entity); }));
    };
}

void test_add_default_component_immediate() {
    "addDefaultComponentById constructs a default component and updates the signature row"_test = [] {
        ECS::Registry registry;
        const auto entity = registry.spawnEntity();
        registry.registerComponent<IdPosition>();

        void* added = Ids::addDefaultComponentById<IdPosition>(registry, entity);

        expect(registry.hasComponents<IdPosition>(entity));
        expect(added == &registry.getComponent<IdPosition>(entity));
        expect(registry.getComponent<IdPosition>(entity).x == 7_i);
    };
}

void test_add_default_component_replaces() {
    "addDefaultComponentById replaces an existing component with a default one"_test = [] {
        ECS::Registry registry;
        const auto entity = registry.spawnEntity();
        registry.addComponent(entity, IdPosition{1, 1});

        (void)Ids::addDefaultComponentById<IdPosition>(registry, entity);

        expect(registry.getComponent<IdPosition>(entity).x == 7_i);
        expect(registry.getComponents<IdPosition>().size() == 1_ul);
    };
}

void test_add_default_component_errors() {
    "addDefaultComponentById needs a live entity, a pool and a default constructor"_test = [] {
        ECS::Registry registry;
        const auto entity = registry.spawnEntity();
        registry.registerComponent<IdNoDefault>();

        expect(throws<std::out_of_range>([&] { (void)Ids::addDefaultComponentById<IdVelocity>(registry, entity); }));
        expect(Ids::addDefaultComponentById<IdNoDefault>(registry, entity) == nullptr);
        expect(!registry.hasComponents<IdNoDefault>(entity));

        registry.killEntity(entity);
        expect(throws<std::invalid_argument>([&] { (void)Ids::addDefaultComponentById<IdNoDefault>(registry, entity); }));
    };
}

void test_remove_component_by_id() {
    "removeComponentById removes the component and fires onRemove"_test = [] {
        ECS::Registry registry;
        const auto entity = registry.spawnEntity();
        int removedX = 0;
        registry.onRemove<IdPosition>([&](IdPosition& position) { removedX = position.x; });
        registry.addComponent(entity, IdPosition{5, 6});

        Ids::removeComponentById(registry, entity, idOf<IdPosition>());

        expect(!registry.hasComponents<IdPosition>(entity));
        expect(removedX == 5_i);
        Ids::removeComponentById(registry, entity, idOf<IdVelocity>());   // no pool: no-op

        registry.killEntity(entity);
        expect(throws<std::invalid_argument>([&] { Ids::removeComponentById(registry, entity, idOf<IdPosition>()); }));
    };
}

void test_component_ids_of() {
    "componentIdsOf lists the owned components in ascending order"_test = [] {
        ECS::Registry registry;
        const auto entity = registry.spawnEntity();
        registry.addComponent(entity, IdVelocity{});
        registry.addComponent(entity, IdPosition{});

        std::vector<ECS::ComponentId> expected{idOf<IdPosition>(), idOf<IdVelocity>()};
        if (expected[0] > expected[1]) {
            std::swap(expected[0], expected[1]);
        }
        expect(Ids::componentIdsOf(registry, entity) == expected);

        registry.killEntity(entity);
        expect(Ids::componentIdsOf(registry, entity).empty());
    };
}

void test_deferred_add_default_component() {
    "a deferred addDefaultComponentById returns the staged value, applied at flush"_test = [] {
        ECS::Registry registry;
        const auto entity = registry.spawnEntity();
        registry.registerComponent<IdPosition>();
        bool visibleDuringDefer = true;

        registry.runDeferred([&] {
            auto* staged = static_cast<IdPosition*>(Ids::addDefaultComponentById<IdPosition>(registry, entity));
            staged->x = 42;
            visibleDuringDefer = registry.hasComponents<IdPosition>(entity);
        });

        expect(!visibleDuringDefer);
        expect(registry.getComponent<IdPosition>(entity).x == 42_i);
    };
}

void test_deferred_add_on_pending_and_killed_entities() {
    "a deferred addDefaultComponentById follows spawns and kills of the same batch"_test = [] {
        ECS::Registry registry;
        registry.registerComponent<IdPosition>();
        const auto doomed = registry.spawnEntity();
        ECS::Entity spawned{};

        registry.runDeferred([&] {
            spawned = registry.spawnEntity();
            (void)Ids::addDefaultComponentById<IdPosition>(registry, spawned);
            (void)Ids::addDefaultComponentById<IdPosition>(registry, doomed);
            registry.killEntity(doomed);
        });

        expect(registry.hasComponents<IdPosition>(spawned));
        expect(registry.getComponents<IdPosition>().size() == 1_ul);
    };
}

void test_deferred_remove_component_by_id() {
    "a deferred removeComponentById applies at flush, unless the entity died"_test = [] {
        ECS::Registry registry;
        const auto entity = registry.spawnEntity();
        const auto doomed = registry.spawnEntity();
        registry.addComponent(entity, IdPosition{});
        registry.addComponent(doomed, IdPosition{});
        bool hasDuringDefer = false;

        registry.runDeferred([&] {
            Ids::removeComponentById(registry, entity, idOf<IdPosition>());
            hasDuringDefer = registry.hasComponents<IdPosition>(entity);
            registry.killEntity(doomed);
            Ids::removeComponentById(registry, doomed, idOf<IdPosition>());
        });

        expect(hasDuringDefer);
        expect(!registry.hasComponents<IdPosition>(entity));
        expect(registry.getComponents<IdPosition>().size() == 0_ul);
    };
}

}

void run_registry_component_id_tests() {
    test_ensure_pool_creates_once();
    test_has_components_by_id();
    test_get_component_by_id();
    test_add_default_component_immediate();
    test_add_default_component_replaces();
    test_add_default_component_errors();
    test_remove_component_by_id();
    test_component_ids_of();
    test_deferred_add_default_component();
    test_deferred_add_on_pending_and_killed_entities();
    test_deferred_remove_component_by_id();
}
