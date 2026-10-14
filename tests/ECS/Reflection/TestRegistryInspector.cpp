#include <boost/ut.hpp>

#include <algorithm>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "ECS/Reflection/Reflection.hpp"
#include "../Support/RegistryTestAccess.hpp"

using namespace boost::ut;

// The catalog and the TypeRegistry are process-wide: names are prefixed with
// "Cr" so they never collide with another test file's E_REFLECT.
namespace {
    struct CrPosition { float x; float y; };
    struct CrHealth { int current = 100; int max = 100; };
    struct CrNoDefault {
        explicit CrNoDefault(int v) : value(v) {}
        int value;
    };
    struct CrManual { int a = 3; };      // not reflected: registered by hand
    struct CrUnlisted { int b; };        // neither reflected nor registered
    struct CrConstant { const int version; };   // reflected, but cannot live in a pool

    namespace first { struct CrDup { int x; }; }
    namespace second { struct CrDup { int y; }; }
}

E_REFLECT(CrPosition)
    E_FIELD(x, float)
    E_FIELD(y, float)
E_END

E_REFLECT(CrHealth)
    E_FIELD(current, int)
    E_FIELD(max, int)
E_END

E_REFLECT(CrNoDefault)
    E_FIELD(value, int)
E_END

E_REFLECT(CrConstant)
    E_FIELD(version, const int)
E_END

E_REFLECT(first::CrDup)
    E_FIELD(x, int)
E_END

// Hand-written Descriptor reusing first::CrDup's name, to make its automatic
// registration fail on purpose.
template<> struct ECS::reflect::Descriptor<second::CrDup> {
    static constexpr std::string_view name = "first::CrDup";
    template<typename Visitor> static void forEach(Visitor&) {}
};

using ECS::reflect::Ref;
using ECS::reflect::RegistryInspector;
using ECS::reflect::TypeInfo;
using ECS::reflect::TypeKind;
using ECS::reflect::TypeRegistry;
using ECS::reflect::detail::ComponentCatalog;
using ECS::TestAccess::RegistryIdAccess;

namespace {
    const TypeInfo& typeNamed(std::string_view name) {
        const TypeInfo* type = TypeRegistry::instance().find(name);
        expect((type != nullptr) >> fatal) << "type not registered:" << std::string(name);
        return *type;
    }

    bool listed(const TypeInfo& type) {
        const auto types = RegistryInspector::types();
        return std::find(types.begin(), types.end(), &type) != types.end();
    }

void test_inspect_is_the_only_way_to_get_an_inspector() {
    "Registry::inspect returns a handle on that registry, the only way to get an inspector"_test = [] {
        static_assert(!std::is_constructible_v<RegistryInspector, ECS::Registry&>);
        ECS::Registry registry;
        const auto entity = registry.spawnEntity();
        registry.addComponent(entity, CrHealth{});

        expect(registry.inspect().get(entity, typeNamed("CrHealth")).data() ==
               &registry.getComponent<CrHealth>(entity));
    };
}

void test_reflected_types_are_registered_before_main() {
    "every E_REFLECT type is a component before main, without any registration call"_test = [] {
        const ECS::reflect::detail::ComponentInfo* info =
            ComponentCatalog::instance().find(ECS::reflect::typeId("CrHealth"));

        expect((info != nullptr) >> fatal);
        expect(info->type == TypeRegistry::instance().find("CrHealth"));
        expect(info->id == ECS::componentId<CrHealth>());
        expect(info->ops.makePool != nullptr && info->ops.insertMoved != nullptr);
        expect(listed(*info->type));
        expect(ComponentCatalog::instance().find(info->id) == info);
        expect(RegistryInspector::errors().empty());
    };
}

void test_add_component_by_type() {
    "add creates the pool and a default component"_test = [] {
        ECS::Registry registry;
        auto inspector = registry.inspect();
        const auto entity = registry.spawnEntity();

        Ref health = inspector.add(entity, typeNamed("CrHealth"));
        expect((health.valid()) >> fatal);
        health.path("current").as<int>() -= 30;

        expect(registry.hasComponents<CrHealth>(entity));
        expect(registry.getComponent<CrHealth>(entity).current == 70_i);
        expect(registry.getComponent<CrHealth>(entity).max == 100_i);
    };
}

void test_get_component_by_type() {
    "get returns a Ref to the typed component"_test = [] {
        ECS::Registry registry;
        auto inspector = registry.inspect();
        const auto entity = registry.spawnEntity();
        const auto other = registry.spawnEntity();
        registry.addComponent(entity, CrPosition{1.0f, 2.0f});

        Ref position = inspector.get(entity, typeNamed("CrPosition"));

        expect(position.data() == &registry.getComponent<CrPosition>(entity));
        expect(position.path("y").as<float>() == 2.0_f);
        expect(!inspector.get(other, typeNamed("CrPosition")).valid());
        expect(!inspector.get(entity, typeNamed("CrHealth")).valid());
        expect(!inspector.get(entity, typeNamed("float")).valid());   // not a component
    };
}

void test_has_and_remove_component_by_type() {
    "has and remove, with onRemove"_test = [] {
        ECS::Registry registry;
        auto inspector = registry.inspect();
        const auto entity = registry.spawnEntity();
        int removed = 0;
        registry.onRemove<CrHealth>([&](CrHealth&) { ++removed; });
        registry.addComponent(entity, CrHealth{});
        const TypeInfo& health = typeNamed("CrHealth");

        expect(inspector.has(entity, health));
        expect(!inspector.has(entity, typeNamed("float")));

        inspector.remove(entity, health);
        inspector.remove(entity, typeNamed("float"));   // not a component: no-op

        expect(!inspector.has(entity, health));
        expect(!registry.hasComponents<CrHealth>(entity));
        expect(removed == 1_i);
    };
}

void test_components_of() {
    "list returns the components of an entity"_test = [] {
        ECS::Registry registry;
        auto inspector = registry.inspect();
        const auto entity = registry.spawnEntity();
        registry.addComponent(entity, CrHealth{});
        registry.addComponent(entity, CrUnlisted{1});
        registry.addComponent(entity, CrPosition{4.0f, 5.0f});

        const std::vector<Ref> components = inspector.list(entity);

        expect((components.size() == 2_ul) >> fatal);   // CrUnlisted is not in the catalog
        expect(&components[0].type() == &typeNamed("CrPosition"));   // ComponentId order
        expect(components[0].data() == &registry.getComponent<CrPosition>(entity));
        expect(&components[1].type() == &typeNamed("CrHealth"));

        registry.killEntity(entity);
        expect(inspector.list(entity).empty());
    };
}

void test_deferred_add_and_remove_by_type() {
    "add and remove follow runDeferred"_test = [] {
        ECS::Registry registry;
        auto inspector = registry.inspect();
        const auto entity = registry.spawnEntity();
        registry.addComponent(entity, CrPosition{});
        bool visibleDuringDefer = true;

        registry.runDeferred([&] {
            inspector.add(entity, typeNamed("CrHealth")).path("max").as<int>() = 250;
            inspector.remove(entity, typeNamed("CrPosition"));
            visibleDuringDefer = registry.hasComponents<CrHealth>(entity);
        });

        expect(!visibleDuringDefer);
        expect(registry.getComponent<CrHealth>(entity).max == 250_i);
        expect(!registry.hasComponents<CrPosition>(entity));
    };
}

void test_component_without_default_constructor() {
    "a component without default constructor cannot be added by type, but is still reachable"_test = [] {
        ECS::Registry registry;
        auto inspector = registry.inspect();
        const auto entity = registry.spawnEntity();
        const TypeInfo& type = typeNamed("CrNoDefault");

        expect(!inspector.add(entity, type).valid());
        expect(!registry.hasComponents<CrNoDefault>(entity));
        expect(!RegistryIdAccess::hasPoolById(registry, ECS::componentId<CrNoDefault>()));   // nothing created

        registry.addComponent(entity, CrNoDefault{9});
        expect(inspector.get(entity, type).path("value").as<int>() == 9_i);
        inspector.remove(entity, type);
        expect(!registry.hasComponents<CrNoDefault>(entity));
    };
}

void test_dead_entity_throws_whatever_the_type() {
    "get, add and remove throw for a dead entity, component type or not"_test = [] {
        ECS::Registry registry;
        auto inspector = registry.inspect();
        const auto entity = registry.spawnEntity();
        registry.killEntity(entity);

        for (const TypeInfo* type : {&typeNamed("float"), &typeNamed("CrConstant"), &typeNamed("CrHealth")}) {
            expect(throws<std::invalid_argument>([&] { (void)inspector.get(entity, *type); })) << type->name;
            expect(throws<std::invalid_argument>([&] { (void)inspector.add(entity, *type); })) << type->name;
            expect(throws<std::invalid_argument>([&] { inspector.remove(entity, *type); })) << type->name;
        }
    };
}

void test_add_on_dead_entity_creates_no_pool() {
    "add on a dead entity throws before creating the pool"_test = [] {
        ECS::Registry registry;
        auto inspector = registry.inspect();
        const auto entity = registry.spawnEntity();
        registry.killEntity(entity);

        expect(throws<std::invalid_argument>([&] { (void)inspector.add(entity, typeNamed("CrPosition")); }));
        expect(!RegistryIdAccess::hasPoolById(registry, ECS::componentId<CrPosition>()));
    };
}

void test_has_on_dead_entity_does_not_throw() {
    "has returns false for a dead entity, component type or not"_test = [] {
        ECS::Registry registry;
        auto inspector = registry.inspect();
        const auto entity = registry.spawnEntity();
        registry.addComponent(entity, CrHealth{});
        registry.killEntity(entity);

        bool hasHealth = true;
        bool hasFloat = true;
        expect(nothrow([&] { hasHealth = inspector.has(entity, typeNamed("CrHealth")); }));
        expect(nothrow([&] { hasFloat = inspector.has(entity, typeNamed("float")); }));
        expect(!hasHealth);
        expect(!hasFloat);
    };
}

void test_non_component_type_on_alive_entity() {
    "a type that is not a component gives an invalid Ref and changes nothing"_test = [] {
        ECS::Registry registry;
        auto inspector = registry.inspect();
        const auto entity = registry.spawnEntity();
        registry.addComponent(entity, CrPosition{1.0f, 2.0f});

        for (const TypeInfo* type : {&typeNamed("float"), &typeNamed("CrConstant")}) {
            Ref added;
            expect(nothrow([&] { added = inspector.add(entity, *type); })) << type->name;
            expect(!added.valid()) << type->name;
            expect(!inspector.get(entity, *type).valid()) << type->name;
            expect(nothrow([&] { inspector.remove(entity, *type); })) << type->name;
        }
        expect(inspector.list(entity).size() == 1_ul);
        expect(registry.getComponent<CrPosition>(entity).y == 2.0_f);
    };
}

void test_manual_registration() {
    "registerComponentType makes any type usable by TypeInfo"_test = [] {
        expect(ComponentCatalog::instance().find(ECS::componentId<CrManual>()) == nullptr);

        const TypeInfo& manual = ECS::reflect::registerComponentType<CrManual>();
        expect(&ECS::reflect::registerComponentType<CrManual>() == &manual);   // idempotent
        expect(manual.kind == TypeKind::Opaque);
        expect(listed(manual));

        ECS::Registry registry;
        auto inspector = registry.inspect();
        const auto entity = registry.spawnEntity();
        expect(inspector.add(entity, manual).valid());
        expect(registry.getComponent<CrManual>(entity).a == 3_i);
    };
}

void test_unstorable_type_is_reflected_but_not_a_component() {
    "a reflected type that cannot live in a pool is reflected but not a component"_test = [] {
        static_assert(!ECS::reflect::ComponentStorable<CrConstant>);
        const TypeInfo& constant = typeNamed("CrConstant");

        expect(constant.kind == TypeKind::Struct);
        expect(constant.findField("version")->readOnly);
        expect(constant.findField("version")->type == TypeRegistry::instance().find<int>());

        const CrConstant value{4};
        expect(Ref::make(const_cast<CrConstant&>(value)).path("version").as<int>() == 4_i);
        expect(ComponentCatalog::instance().find(constant.id) == nullptr);
        expect(!listed(constant));

        ECS::Registry registry;
        auto inspector = registry.inspect();
        const auto entity = registry.spawnEntity();
        expect(!inspector.add(entity, constant).valid());
    };
}

void test_find_component_type() {
    "findType only returns types usable as components"_test = [] {
        expect(RegistryInspector::findType("CrHealth") == &typeNamed("CrHealth"));
        expect(RegistryInspector::findType("float") == nullptr);           // reflected, not a component
        expect(RegistryInspector::findType("CrConstant") == nullptr);      // reflected, not storable
        expect(RegistryInspector::findType("CrUnknown") == nullptr);
    };
}

void test_registration_errors_are_recorded() {
    "a failing automatic registration is recorded instead of aborting"_test = [] {
        const auto before = RegistryInspector::errors().size();

        ECS::reflect::detail::registerReflectedType<second::CrDup>();   // name already taken

        const auto& errors = RegistryInspector::errors();
        expect((errors.size() == before + 1) >> fatal);
        expect(errors.back().rfind("first::CrDup: ", 0) == 0_ul);
        expect(ComponentCatalog::instance().find(ECS::componentId<second::CrDup>()) == nullptr);
    };
}

void test_entities_lists_the_live_entities() {
    "entities lists the live entities of the registry"_test = [] {
        ECS::Registry world;
        const ECS::Entity first = world.spawnEntity();
        const ECS::Entity second = world.spawnEntity();
        const auto inspector = world.inspect();

        std::vector<ECS::Entity> listed;
        for (ECS::Entity entity : inspector.entities()) {
            listed.push_back(entity);
        }

        expect(listed == std::vector<ECS::Entity>{first, second});
        expect(inspector.entities().size() == 2_ul);
    };
}

void test_entities_is_empty_for_an_empty_registry() {
    "entities is empty for an empty registry"_test = [] {
        ECS::Registry world;
        const auto inspector = world.inspect();

        expect(inspector.entities().empty());
        expect(inspector.entities().begin() == inspector.entities().end());
    };
}

void test_kept_entities_follows_the_world() {
    "a kept entities() list follows spawns, kills and deferred spawns"_test = [] {
        ECS::Registry world;
        const ECS::Entity killed = world.spawnEntity();
        const ECS::EntityList all = world.inspect().entities();

        ECS::Entity spawned{};
        std::size_t countDuring = 0;
        world.runDeferred([&] {
            spawned = world.spawnEntity();
            world.killEntity(killed);
            countDuring = all.size();
        });

        std::vector<ECS::Entity> listed;
        for (ECS::Entity entity : all) {
            listed.push_back(entity);
        }
        expect(countDuring == 1_ul);
        expect(listed == std::vector<ECS::Entity>{spawned});
    };
}

void test_entities_of_two_registries_are_separate() {
    "entities only lists the entities of its own registry"_test = [] {
        ECS::Registry world;
        ECS::Registry other;
        other.spawnEntity();

        expect(world.inspect().entities().empty());
        expect(other.inspect().entities().size() == 1_ul);
    };
}


void test_display_name_returns_the_name() {
    "displayName returns the entity's Name"_test = [] {
        ECS::Registry world;
        const ECS::Entity player = world.spawnEntity("Player");

        expect(world.inspect().displayName(player) == "Player");
    };
}

void test_display_name_falls_back_on_the_index() {
    "displayName falls back on \"Entity <index>\" without a Name"_test = [] {
        ECS::Registry world;
        world.spawnEntity();
        const ECS::Entity tile = world.spawnEntity();

        expect(world.inspect().displayName(tile) == "Entity 1");
    };
}

void test_display_name_treats_an_empty_name_as_no_name() {
    "displayName treats an empty Name as no name"_test = [] {
        ECS::Registry world;
        const ECS::Entity entity = world.spawnEntity();
        world.addComponent(entity, ECS::Name{""});

        expect(world.inspect().displayName(entity) == "Entity 0");
    };
}

void test_display_name_follows_name_changes() {
    "displayName follows a Name that is replaced, removed and added again"_test = [] {
        ECS::Registry world;
        const ECS::Entity entity = world.spawnEntity("First");
        const auto inspector = world.inspect();

        world.getComponent<ECS::Name>(entity).value = "Renamed";
        expect(inspector.displayName(entity) == "Renamed");

        world.removeComponent<ECS::Name>(entity);
        expect(inspector.displayName(entity) == "Entity 0");

        world.addComponent(entity, ECS::Name{"Back"});
        expect(inspector.displayName(entity) == "Back");
    };
}

void test_display_name_rejects_dead_and_foreign_entities() {
    "displayName throws for a dead, foreign or default handle"_test = [] {
        ECS::Registry world;
        ECS::Registry other;
        const ECS::Entity dead = world.spawnEntity("Dead");
        world.killEntity(dead);
        const ECS::Entity foreign = other.spawnEntity("Foreign");
        const auto inspector = world.inspect();

        expect(throws<std::invalid_argument>([&] { (void)inspector.displayName(dead); }));
        expect(throws<std::invalid_argument>([&] { (void)inspector.displayName(foreign); }));
        expect(throws<std::invalid_argument>([&] { (void)inspector.displayName(ECS::Entity{}); }));
    };
}

void test_display_name_of_a_deferred_spawn() {
    "displayName of an entity spawned under runDeferred falls back until the flush"_test = [] {
        ECS::Registry world;
        std::string during;
        ECS::Entity spawned{};

        world.runDeferred([&] {
            spawned = world.spawnEntity("Late");
            during = world.inspect().displayName(spawned);
        });

        expect(during == "Entity 0");
        expect(world.inspect().displayName(spawned) == "Late");
    };
}

void test_name_is_a_reflected_component() {
    "ECS::Name is a reflected component type listed with the entity's components"_test = [] {
        const TypeInfo* name = RegistryInspector::findType("ECS::Name");
        expect((name != nullptr) >> fatal);
        const std::vector<const TypeInfo*> types = RegistryInspector::types();
        expect(std::ranges::find(types, name) != types.end());

        ECS::Registry world;
        const ECS::Entity player = world.spawnEntity("Player");
        const std::vector<Ref> components = world.inspect().list(player);

        expect((components.size() == 1_ul) >> fatal);
        expect(&components[0].type() == name);
        expect(components[0].path("value").as<std::string>() == "Player");
    };
}

}

void run_registry_inspector_tests() {
    test_inspect_is_the_only_way_to_get_an_inspector();
    test_reflected_types_are_registered_before_main();
    test_add_component_by_type();
    test_get_component_by_type();
    test_has_and_remove_component_by_type();
    test_components_of();
    test_deferred_add_and_remove_by_type();
    test_component_without_default_constructor();
    test_dead_entity_throws_whatever_the_type();
    test_add_on_dead_entity_creates_no_pool();
    test_has_on_dead_entity_does_not_throw();
    test_non_component_type_on_alive_entity();
    test_manual_registration();
    test_unstorable_type_is_reflected_but_not_a_component();
    test_find_component_type();
    test_registration_errors_are_recorded();
    test_entities_lists_the_live_entities();
    test_entities_is_empty_for_an_empty_registry();
    test_kept_entities_follows_the_world();
    test_entities_of_two_registries_are_separate();
    test_display_name_returns_the_name();
    test_display_name_falls_back_on_the_index();
    test_display_name_treats_an_empty_name_as_no_name();
    test_display_name_follows_name_changes();
    test_display_name_rejects_dead_and_foreign_entities();
    test_display_name_of_a_deferred_spawn();
    test_name_is_a_reflected_component();
}
