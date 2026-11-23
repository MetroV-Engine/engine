#include <boost/ut.hpp>

#include <string>

#include "Engine/Serialization/Serialization.hpp"
#include "Serialization/Support/Helpers.hpp"
#include "Serialization/Support/TestTypes.hpp"

using namespace boost::ut;
using serial::Report;
using serial::serialize;
using namespace sertest;

namespace {
    ECS::Entity spawnWithUuid(ECS::Registry& world, const std::string& name = {}) {
        const ECS::Entity entity = world.spawnEntity(name);
        world.addComponent(entity, Uuid{uuid::generate()});
        return entity;
    }
}

void run_serialize_world_tests() {
    "an empty world is an empty list of entities"_test = [] {
        ECS::Registry world;
        Report report;
        const Json json = parseJson(serialize(world, &report));
        expect(report.ok());
        expect(json["format"] == "metrov.world");
        expect(json["version"] == serial::FormatVersion);
        expect(json["entities"] == Json::array());
    };

    "an entity is written with its id and its components"_test = [] {
        ECS::Registry world;
        const ECS::Entity player = spawnWithUuid(world, "player");
        world.addComponent(player, Health{75, 100});
        const Uuid id = world.getComponent<Uuid>(player);

        Report report;
        const Json json = parseJson(serialize(world, &report));

        expect(report.ok());
        expect(report.warnings.empty());
        expect(json["entities"].size() == std::size_t{1});
        const Json& entity = json["entities"][0];
        expect(entity["id"] == uuid::toString(id));
        expect(entity["components"]["ECS::Name"]["value"] == "player");
        expect(entity["components"]["sertest::Health"]["current"] == 75);
    };

    "the Uuid is the id of the entity, not one of its components"_test = [] {
        ECS::Registry world;
        spawnWithUuid(world);
        const Json json = parseJson(serialize(world));
        expect(!json["entities"][0]["components"].contains("Uuid"));
    };

    "an entity without components has an empty object"_test = [] {
        ECS::Registry world;
        spawnWithUuid(world);
        const Json json = parseJson(serialize(world));
        expect(json["entities"][0]["components"] == Json::object());
    };

    "a component with no field is written as an empty object"_test = [] {
        ECS::Registry world;
        const ECS::Entity entity = spawnWithUuid(world);
        world.addComponent(entity, Marker{});
        const Json json = parseJson(serialize(world));
        expect(json["entities"][0]["components"]["sertest::Marker"] == Json::object());
    };

    "entities are written in the order they were created"_test = [] {
        ECS::Registry world;
        const ECS::Entity a = spawnWithUuid(world);
        const ECS::Entity b = spawnWithUuid(world);
        const ECS::Entity c = spawnWithUuid(world);
        const Json json = parseJson(serialize(world));
        expect(json["entities"][0]["id"] == uuid::toString(world.getComponent<Uuid>(a)));
        expect(json["entities"][1]["id"] == uuid::toString(world.getComponent<Uuid>(b)));
        expect(json["entities"][2]["id"] == uuid::toString(world.getComponent<Uuid>(c)));
    };

    "a killed entity is not written"_test = [] {
        ECS::Registry world;
        const ECS::Entity a = spawnWithUuid(world);
        spawnWithUuid(world);
        world.killEntity(a);
        expect(parseJson(serialize(world))["entities"].size() == std::size_t{1});
    };

    // ---- entities without an id

    "an entity without a Uuid gets a temporary id in the text only"_test = [] {
        ECS::Registry world;
        const ECS::Entity entity = world.spawnEntity();
        world.addComponent(entity, Health{});

        const Json json = parseJson(serialize(world));

        const std::string id = json["entities"][0]["id"];
        expect(uuid::fromString(id).has_value());
        expect(!world.hasComponents<Uuid>(entity));   // the world is untouched
    };

    "two entities with the same Uuid do not share an id in the text"_test = [] {
        ECS::Registry world;
        const Uuid shared = uuid::generate();
        const ECS::Entity a = world.spawnEntity();
        const ECS::Entity b = world.spawnEntity();
        world.addComponent(a, Uuid{shared});
        world.addComponent(b, Uuid{shared});

        Report report;
        const Json json = parseJson(serialize(world, &report));

        expect(report.ok());
        expect(hasWarning(report, uuid::toString(shared)));
        expect(json["entities"][0]["id"] == uuid::toString(shared));
        expect(json["entities"][1]["id"] != uuid::toString(shared));
    };

    "a null Uuid counts as no Uuid"_test = [] {
        ECS::Registry world;
        const ECS::Entity entity = world.spawnEntity();
        world.addComponent(entity, Uuid{});
        const Json json = parseJson(serialize(world));
        const auto id = uuid::fromString(json["entities"][0]["id"].get<std::string>());
        expect(id.has_value());
        expect(!uuid::isNull(*id));
    };

    // ---- references between entities

    "a reference is written as the id of its target"_test = [] {
        ECS::Registry world;
        const ECS::Entity target = spawnWithUuid(world);
        const ECS::Entity holder = spawnWithUuid(world);
        Link link;
        link.target = target;
        link.others = {target, holder};
        world.addComponent(holder, std::move(link));

        Report report;
        const Json json = parseJson(serialize(world, &report));

        const std::string targetId = uuid::toString(world.getComponent<Uuid>(target));
        const std::string holderId = uuid::toString(world.getComponent<Uuid>(holder));
        const Json& written = json["entities"][1]["components"]["sertest::Link"];
        expect(report.warnings.empty());
        expect(written["target"] == targetId);
        expect(written["others"][0] == targetId);
        expect(written["others"][1] == holderId);
    };

    "a reference to an entity without a Uuid uses its temporary id"_test = [] {
        ECS::Registry world;
        const ECS::Entity target = world.spawnEntity();
        const ECS::Entity holder = world.spawnEntity();
        world.addComponent(holder, Link{target, {}});

        const Json json = parseJson(serialize(world));

        expect(json["entities"][1]["components"]["sertest::Link"]["target"] == json["entities"][0]["id"]);
    };

    "a reference to itself is written as its own id"_test = [] {
        ECS::Registry world;
        const ECS::Entity entity = spawnWithUuid(world);
        world.addComponent(entity, Link{entity, {}});
        const Json json = parseJson(serialize(world));
        expect(json["entities"][0]["components"]["sertest::Link"]["target"] == json["entities"][0]["id"]);
    };

    "a reference to a killed entity is null with a warning"_test = [] {
        ECS::Registry world;
        const ECS::Entity target = spawnWithUuid(world);
        const ECS::Entity holder = spawnWithUuid(world);
        world.addComponent(holder, Link{target, {}});
        world.killEntity(target);

        Report report;
        const Json json = parseJson(serialize(world, &report));

        expect(json["entities"][0]["components"]["sertest::Link"]["target"].is_null());
        expect(hasWarning(report, "sertest::Link.target"));
    };

    "a reference to an entity of another world is null with a warning"_test = [] {
        ECS::Registry world;
        ECS::Registry other;
        const ECS::Entity foreign = other.spawnEntity();
        const ECS::Entity holder = spawnWithUuid(world);
        world.addComponent(holder, Link{foreign, {}});

        Report report;
        const Json json = parseJson(serialize(world, &report));

        expect(json["entities"][0]["components"]["sertest::Link"]["target"].is_null());
        expect(hasWarning(report, "sertest::Link.target"));
    };

    "in a world, a handle that refers to nothing is null without a warning"_test = [] {
        ECS::Registry world;
        const ECS::Entity holder = spawnWithUuid(world);
        world.addComponent(holder, Link{});
        Report report;
        const Json json = parseJson(serialize(world, &report));
        expect(json["entities"][0]["components"]["sertest::Link"]["target"].is_null());
        expect(report.warnings.empty());
    };

    // ---- what cannot be written

    "a field that cannot be written is skipped with a warning naming the entity"_test = [] {
        ECS::Registry world;
        const ECS::Entity entity = spawnWithUuid(world);
        world.addComponent(entity, WithBlob{});
        Report report;
        const Json json = parseJson(serialize(world, &report));
        expect(report.ok());
        expect(json["entities"][0]["components"]["sertest::WithBlob"].contains("kept"));
        expect(hasWarning(report, uuid::toString(world.getComponent<Uuid>(entity))));
    };

    "a text that cannot be produced is an error and an empty string"_test = [] {
        ECS::Registry world;
        const ECS::Entity entity = spawnWithUuid(world);
        Item item;
        item.id = "\xFF";
        world.addComponent(entity, Item{item});
        Report report;
        expect(serialize(world, &report).empty());
        expect(!report.ok());
    };

    "serialize does not change the world"_test = [] {
        ECS::Registry world;
        const ECS::Entity entity = world.spawnEntity("x");
        world.addComponent(entity, Health{3, 4});
        const std::size_t count = world.entityCount();
        (void)serialize(world);
        (void)serialize(world);
        expect(world.entityCount() == count);
        expect(world.getComponent<Health>(entity).current == 3);
    };

    "serialize of a world works without a report"_test = [] {
        ECS::Registry world;
        expect(!serialize(world).empty());
    };

    // ---- assignMissingUuids

    "assignMissingUuids gives the entities without a Uuid the id written in the text"_test = [] {
        ECS::Registry world;
        const ECS::Entity entity = world.spawnEntity("scenery");
        serial::SaveOptions options;
        options.assignMissingUuids = true;

        const Json json = parseJson(serialize(world, nullptr, options));

        expect(world.hasComponents<Uuid>(entity));
        expect(json["entities"][0]["id"] == uuid::toString(world.getComponent<Uuid>(entity)));
    };

    "with assignMissingUuids, saving twice writes the same text"_test = [] {
        ECS::Registry world;
        const ECS::Entity a = world.spawnEntity("a");
        world.spawnEntity("b");
        world.addComponent(a, Health{1, 2});
        serial::SaveOptions options;
        options.assignMissingUuids = true;

        const std::string first = serialize(world, nullptr, options);
        const std::string second = serialize(world, nullptr, options);
        const std::string third = serialize(world);   // the ids are the entities' own now

        expect(first == second);
        expect(first == third);
    };

    "without assignMissingUuids, saving twice writes different temporary ids"_test = [] {
        ECS::Registry world;
        world.spawnEntity();
        expect(serialize(world) != serialize(world));
    };

    "assignMissingUuids replaces a null Uuid"_test = [] {
        ECS::Registry world;
        const ECS::Entity entity = world.spawnEntity();
        world.addComponent(entity, Uuid{});
        serial::SaveOptions options;
        options.assignMissingUuids = true;
        (void)serialize(world, nullptr, options);
        expect(!uuid::isNull(world.getComponent<Uuid>(entity)));
    };

    "assignMissingUuids keeps the Uuid an entity already has"_test = [] {
        ECS::Registry world;
        const ECS::Entity entity = spawnWithUuid(world);
        const Uuid before = world.getComponent<Uuid>(entity);
        serial::SaveOptions options;
        options.assignMissingUuids = true;
        (void)serialize(world, nullptr, options);
        expect(world.getComponent<Uuid>(entity) == before);
    };

    "assignMissingUuids leaves a duplicated Uuid alone, with a warning"_test = [] {
        ECS::Registry world;
        const Uuid shared = uuid::generate();
        const ECS::Entity a = world.spawnEntity();
        const ECS::Entity b = world.spawnEntity();
        world.addComponent(a, Uuid{shared});
        world.addComponent(b, Uuid{shared});
        serial::SaveOptions options;
        options.assignMissingUuids = true;

        Report report;
        (void)serialize(world, &report, options);

        expect(hasWarning(report, uuid::toString(shared)));
        expect(world.getComponent<Uuid>(a) == shared);
        expect(world.getComponent<Uuid>(b) == shared);   // not rewritten behind the user's back
    };

    "a reference to an entity given a Uuid by assignMissingUuids uses that Uuid"_test = [] {
        ECS::Registry world;
        const ECS::Entity target = world.spawnEntity();
        const ECS::Entity holder = world.spawnEntity();
        world.addComponent(holder, Link{target, {}});
        serial::SaveOptions options;
        options.assignMissingUuids = true;

        const Json json = parseJson(serialize(world, nullptr, options));

        const std::string id = uuid::toString(world.getComponent<Uuid>(target));
        expect(json["entities"][1]["components"]["sertest::Link"]["target"] == id);
    };

    "assignMissingUuids on an empty world does nothing"_test = [] {
        ECS::Registry world;
        serial::SaveOptions options;
        options.assignMissingUuids = true;
        Report report;
        expect(!serialize(world, &report, options).empty());
        expect(report.ok());
        expect(world.entityCount() == std::size_t{0});
    };

    "serialize returns its text when the report already holds an error"_test = [] {
        ECS::Registry world;
        spawnWithUuid(world);
        Report report;
        report.errors.push_back("from an earlier call");
        expect(!serialize(world, &report).empty());
    };
}
