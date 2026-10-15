#include <boost/ut.hpp>

#include <algorithm>
#include <string>
#include <vector>

#include "ECS/Reflection/PrettyPrint.hpp"
#include "ECS/Reflection/Reflection.hpp"

using namespace boost::ut;

// End-to-end check that reflection works on a type this file does not know.
//
// The Treasure component is defined in Treasure.cpp, which this file does not
// include: there is no Treasure type here, only its name. Everything below goes
// through world.inspect() and Ref, as an editor, a scripting binding or a scene
// loader would.

using ECS::reflect::Ref;
using ECS::reflect::TypeInfo;
using ECS::reflect::TypeKind;

namespace {
    const TypeInfo& treasureType(ECS::Registry& world) {
        const TypeInfo* type = world.inspect().findType("Treasure");
        expect((type != nullptr) >> fatal) << "Treasure was not registered before main";
        return *type;
    }

void test_unknown_type_is_discovered_by_name() {
    "a type defined in another file is discovered by name, with its fields"_test = [] {
        ECS::Registry world;
        const TypeInfo& treasure = treasureType(world);

        expect(treasure.kind == TypeKind::Struct);
        expect((treasure.fields.size() == 4_ul) >> fatal);
        expect(treasure.fields[0].name == "name" && treasure.fields[0].type->name == "string");
        expect(treasure.fields[1].name == "gold" && treasure.fields[1].type->name == "int32");
        expect(treasure.fields[2].name == "gems" && treasure.fields[2].type->name == "vector<string>");
        expect(treasure.fields[3].name == "spot" && treasure.fields[3].type->name == "TreasureSpot");

        const auto types = world.inspect().types();
        expect(std::find(types.begin(), types.end(), &treasure) != types.end());
    };
}

void test_unknown_component_is_created_and_modified() {
    "a component of an unknown type is added, modified and read back"_test = [] {
        ECS::Registry world;
        const auto entity = world.spawnEntity();
        auto inspector = world.inspect();
        const TypeInfo& treasure = treasureType(world);

        Ref chest = inspector.add(entity, treasure);
        expect((chest.valid()) >> fatal);
        expect(chest.path("gold").as<int>() == 0_i);   // default member initializer of the unknown type

        chest.path("name").as<std::string>() = "chest";
        chest.path("gold").as<int>() = 250;
        chest.path("gems").push().as<std::string>() = "ruby";
        chest.path("spot.x").as<float>() = 1.0f;
        chest.path("spot.y").as<float>() = 2.0f;

        Ref again = inspector.get(entity, treasure);   // fetched anew, from the registry
        expect(inspector.has(entity, treasure));
        expect(again.path("name").as<std::string>() == std::string("chest"));
        expect(again.path("gold").as<int>() == 250_i);
        expect(again.path("gems").size() == 1_ul);
        expect(again.path("gems[0]").as<std::string>() == std::string("ruby"));
        expect(ECS::reflect::toPrettyString(again, {.indent = 0}) == std::string(
            "Treasure { name: \"chest\", gold: 250, gems: [ \"ruby\" ], spot: TreasureSpot { x: 1, y: 2 } }"));
    };
}

void test_unknown_component_is_listed_and_removed() {
    "a component of an unknown type is listed with the others, then removed"_test = [] {
        ECS::Registry world;
        const auto entity = world.spawnEntity();
        auto inspector = world.inspect();
        const TypeInfo& treasure = treasureType(world);
        inspector.add(entity, treasure);
        inspector.add(entity, *inspector.findType("TreasureSpot"));   // also a component

        const std::vector<Ref> components = inspector.list(entity);
        expect(components.size() == 2_ul);
        expect(std::any_of(components.begin(), components.end(),
                           [&](const Ref& c) { return &c.type() == &treasure; }));

        inspector.remove(entity, treasure);
        expect(!inspector.has(entity, treasure));
        expect(inspector.list(entity).size() == 1_ul);
    };
}

void test_unknown_component_in_deferred_mode() {
    "a component of an unknown type added inside a system is applied at flush"_test = [] {
        ECS::Registry world;
        const auto entity = world.spawnEntity();
        auto inspector = world.inspect();
        const TypeInfo& treasure = treasureType(world);
        bool visibleDuringSystem = true;

        world.runDeferred([&] {
            inspector.add(entity, treasure).path("gold").as<int>() = 99;
            visibleDuringSystem = inspector.has(entity, treasure);
        });

        expect(!visibleDuringSystem);
        expect(inspector.get(entity, treasure).path("gold").as<int>() == 99_i);
    };
}

}

void run_unknown_type_tests() {
    test_unknown_type_is_discovered_by_name();
    test_unknown_component_is_created_and_modified();
    test_unknown_component_is_listed_and_removed();
    test_unknown_component_in_deferred_mode();
}
