#include <boost/ut.hpp>

#include <stdexcept>
#include <vector>

#include "ECS/ComponentPool.hpp"
#include "ECS/Registry.hpp"

using namespace boost::ut;

// ComponentId-based access of the Registry: the type-erased core used by the
// reflection layer, which the typed has/remove API forwards to.
namespace {
    struct IdPosition { int x = 7; int y = 8; };
    struct IdVelocity { int dx = 0; };
    struct IdNoDefault {
        explicit IdNoDefault(int v) : value(v) {}
        int value;
    };

    template<typename T>
    ECS::ComponentId idOf() { return ECS::componentId<T>(); }

void test_ensure_pool_creates_once() {
    "ensurePool creates the pool once and keeps an existing one"_test = [] {
        ECS::Registry registry;
        const auto entity = registry.spawnEntity();

        expect(!registry.hasPool(idOf<IdPosition>()));
        registry.ensurePool(idOf<IdPosition>(), &ECS::ComponentPool<IdPosition>::create);
        expect(registry.hasPool(idOf<IdPosition>()));

        registry.addComponent(entity, IdPosition{1, 2});
        registry.ensurePool(idOf<IdPosition>(), &ECS::ComponentPool<IdPosition>::create);
        expect(registry.getComponent<IdPosition>(entity).x == 1_i);
    };
}

void test_ensure_pool_rejects_ids_beyond_signature() {
    "ensurePool rejects an id that does not fit in a Signature"_test = [] {
        ECS::Registry registry;

        expect(throws<std::out_of_range>([&] {
            registry.ensurePool(static_cast<ECS::ComponentId>(ECS::MaxComponentTypes),
                                &ECS::ComponentPool<IdPosition>::create);
        }));
    };
}

void test_has_components_by_id() {
    "hasComponents by id agrees with the typed overload"_test = [] {
        ECS::Registry registry;
        const auto entity = registry.spawnEntity();
        registry.addComponent(entity, IdPosition{});

        expect(registry.hasComponents(entity, {idOf<IdPosition>()}));
        expect(!registry.hasComponents(entity, {idOf<IdPosition>(), idOf<IdVelocity>()}));
        expect(registry.hasComponents(entity, {}));
        expect(!registry.hasComponents(entity, {static_cast<ECS::ComponentId>(ECS::MaxComponentTypes)}));

        registry.killEntity(entity);
        expect(!registry.hasComponents(entity, {idOf<IdPosition>()}));
    };
}

void test_get_component_by_id() {
    "getComponent by id returns the component address or nullptr"_test = [] {
        ECS::Registry registry;
        const auto entity = registry.spawnEntity();
        const auto other = registry.spawnEntity();
        registry.addComponent(entity, IdPosition{3, 4});

        expect(registry.getComponent(entity, idOf<IdPosition>()) == &registry.getComponent<IdPosition>(entity));
        expect(registry.getComponent(other, idOf<IdPosition>()) == nullptr);   // no component
        expect(registry.getComponent(entity, idOf<IdVelocity>()) == nullptr);  // no pool

        registry.killEntity(entity);
        expect(throws<std::invalid_argument>([&] { (void)registry.getComponent(entity, idOf<IdPosition>()); }));
    };
}

void test_add_default_component_immediate() {
    "addDefaultComponent constructs a default component and updates the Signature"_test = [] {
        ECS::Registry registry;
        const auto entity = registry.spawnEntity();
        registry.registerComponent<IdPosition>();

        void* added = registry.addDefaultComponent(entity, idOf<IdPosition>());

        expect(registry.hasComponents<IdPosition>(entity));
        expect(added == &registry.getComponent<IdPosition>(entity));
        expect(registry.getComponent<IdPosition>(entity).x == 7_i);
    };
}

void test_add_default_component_replaces() {
    "addDefaultComponent replaces an existing component with a default one"_test = [] {
        ECS::Registry registry;
        const auto entity = registry.spawnEntity();
        registry.addComponent(entity, IdPosition{1, 1});

        (void)registry.addDefaultComponent(entity, idOf<IdPosition>());

        expect(registry.getComponent<IdPosition>(entity).x == 7_i);
        expect(registry.getComponents<IdPosition>().size() == 1_ul);
    };
}

void test_add_default_component_errors() {
    "addDefaultComponent needs a live entity, a pool and a default constructor"_test = [] {
        ECS::Registry registry;
        const auto entity = registry.spawnEntity();
        registry.registerComponent<IdNoDefault>();

        expect(throws<std::out_of_range>([&] { (void)registry.addDefaultComponent(entity, idOf<IdVelocity>()); }));
        expect(registry.addDefaultComponent(entity, idOf<IdNoDefault>()) == nullptr);
        expect(!registry.hasComponents<IdNoDefault>(entity));

        registry.killEntity(entity);
        expect(throws<std::invalid_argument>([&] { (void)registry.addDefaultComponent(entity, idOf<IdNoDefault>()); }));
    };
}

void test_remove_component_by_id() {
    "removeComponent by id removes the component and fires onRemove"_test = [] {
        ECS::Registry registry;
        const auto entity = registry.spawnEntity();
        int removedX = 0;
        registry.onRemove<IdPosition>([&](IdPosition& position) { removedX = position.x; });
        registry.addComponent(entity, IdPosition{5, 6});

        registry.removeComponent(entity, idOf<IdPosition>());

        expect(!registry.hasComponents<IdPosition>(entity));
        expect(removedX == 5_i);
        registry.removeComponent(entity, idOf<IdVelocity>());   // no pool: no-op

        registry.killEntity(entity);
        expect(throws<std::invalid_argument>([&] { registry.removeComponent(entity, idOf<IdPosition>()); }));
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
        expect(registry.componentIdsOf(entity) == expected);

        registry.killEntity(entity);
        expect(registry.componentIdsOf(entity).empty());
    };
}

void test_deferred_add_default_component() {
    "a deferred addDefaultComponent returns the staged value, applied at flush"_test = [] {
        ECS::Registry registry;
        const auto entity = registry.spawnEntity();
        registry.registerComponent<IdPosition>();
        bool visibleDuringDefer = true;

        registry.runDeferred([&] {
            auto* staged = static_cast<IdPosition*>(registry.addDefaultComponent(entity, idOf<IdPosition>()));
            staged->x = 42;
            visibleDuringDefer = registry.hasComponents<IdPosition>(entity);
        });

        expect(!visibleDuringDefer);
        expect(registry.getComponent<IdPosition>(entity).x == 42_i);
    };
}

void test_deferred_add_on_pending_and_killed_entities() {
    "a deferred addDefaultComponent follows spawns and kills of the same batch"_test = [] {
        ECS::Registry registry;
        registry.registerComponent<IdPosition>();
        const auto doomed = registry.spawnEntity();
        ECS::Entity spawned{};

        registry.runDeferred([&] {
            spawned = registry.spawnEntity();
            (void)registry.addDefaultComponent(spawned, idOf<IdPosition>());
            (void)registry.addDefaultComponent(doomed, idOf<IdPosition>());
            registry.killEntity(doomed);
        });

        expect(registry.hasComponents<IdPosition>(spawned));
        expect(registry.getComponents<IdPosition>().size() == 1_ul);
    };
}

void test_deferred_remove_component_by_id() {
    "a deferred removeComponent by id applies at flush, unless the entity died"_test = [] {
        ECS::Registry registry;
        const auto entity = registry.spawnEntity();
        const auto doomed = registry.spawnEntity();
        registry.addComponent(entity, IdPosition{});
        registry.addComponent(doomed, IdPosition{});
        bool hasDuringDefer = false;

        registry.runDeferred([&] {
            registry.removeComponent(entity, idOf<IdPosition>());
            hasDuringDefer = registry.hasComponents<IdPosition>(entity);
            registry.killEntity(doomed);
            registry.removeComponent(doomed, idOf<IdPosition>());
        });

        expect(hasDuringDefer);
        expect(!registry.hasComponents<IdPosition>(entity));
        expect(registry.getComponents<IdPosition>().size() == 0_ul);
    };
}

}

void run_registry_component_id_tests() {
    test_ensure_pool_creates_once();
    test_ensure_pool_rejects_ids_beyond_signature();
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
