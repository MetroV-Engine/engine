#include <boost/ut.hpp>

#include <string>

#include "Engine/Serialization/Serialization.hpp"
#include "Serialization/Support/Helpers.hpp"
#include "Serialization/Support/TestTypes.hpp"

using namespace boost::ut;
using serial::load;
using serial::Report;
using serial::serialize;
using namespace sertest;

namespace {
    const std::string A = "aaaaaaaa-aaaa-4aaa-8aaa-aaaaaaaaaaaa";
    const std::string B = "bbbbbbbb-bbbb-4bbb-8bbb-bbbbbbbbbbbb";
    const std::string C = "cccccccc-cccc-4ccc-8ccc-cccccccccccc";

    std::string entity(const std::string& id, const std::string& components = "{}") {
        return R"({"id":")" + id + R"(","components":)" + components + "}";
    }

    // Finds the entity loaded with this id.
    ECS::Entity find(ECS::Registry& world, const std::string& id) {
        const Uuid wanted = *uuid::fromString(id);
        for (const ECS::Entity candidate : world.inspect().entities())
            if (world.hasComponents<Uuid>(candidate) && world.getComponent<Uuid>(candidate) == wanted)
                return candidate;
        return ECS::Entity{};
    }

    // A world text with a "systems" block (written as given, any JSON).
    std::string sceneText(const std::string& systems, const std::string& entities = "[]") {
        return R"({"format":"metrov.world","version":1,"systems":)" + systems + R"(,"entities":)" + entities + "}";
    }

    // A text that must be refused, and leave the world as it was.
    void expectRefused(const std::string& text, const std::string& errorPart = "") {
        ECS::Registry world;
        world.spawnEntity("existing");
        Report report;
        expect(!load(world, text, &report));
        expect(!report.ok());
        expect(world.entityCount() == std::size_t{1});
        if (!errorPart.empty())
            expect(hasError(report, errorPart));
    }
}

void run_load_world_tests() {
    // ---- success

    "load creates the entities with their id and their components"_test = [] {
        ECS::Registry world;
        Report report;
        const std::string text = worldText("[" + entity(A, R"({"sertest::Health":{"current":7,"max":9},"ECS::Name":{"value":"hero"}})") + "]");

        expect(load(world, text, &report));

        expect(report.ok());
        expect(report.warnings.empty());
        expect(world.entityCount() == std::size_t{1});
        const ECS::Entity hero = find(world, A);
        expect(world.isAlive(hero));
        expect(world.getComponent<Health>(hero).current == 7);
        expect(world.getComponent<Health>(hero).max == 9);
        expect(world.getComponent<ECS::Name>(hero).value == "hero");
    };

    "load of an empty list does nothing"_test = [] {
        ECS::Registry world;
        expect(load(world, worldText("[]")));
        expect(world.entityCount() == std::size_t{0});
    };

    "an entity without components is created with its id"_test = [] {
        ECS::Registry world;
        expect(load(world, worldText("[" + entity(A) + R"(,{"id":")" + B + R"("}])")));
        expect(world.entityCount() == std::size_t{2});
        expect(world.isAlive(find(world, A)));
        expect(world.isAlive(find(world, B)));
    };

    "load adds to the entities already in the world"_test = [] {
        ECS::Registry world;
        const ECS::Entity existing = world.spawnEntity("existing");
        expect(load(world, worldText("[" + entity(A) + "]")));
        expect(world.entityCount() == std::size_t{2});
        expect(world.isAlive(existing));
        expect(world.getComponent<ECS::Name>(existing).value == "existing");
    };

    "a world reads back identical after save and load"_test = [] {
        ECS::Registry source;
        const ECS::Entity a = source.spawnEntity("a");
        source.addComponent(a, Uuid{uuid::generate()});
        source.addComponent(a, Health{1, 2});
        const ECS::Entity b = source.spawnEntity();
        source.addComponent(b, Uuid{uuid::generate()});
        Inventory inventory;
        inventory.items = {{"sword", 2}};
        inventory.counts = {{"gold", 9}};
        inventory.equipped = Item{"bow", 1};
        source.addComponent(b, std::move(inventory));
        source.addComponent(b, Link{a, {a, b}});
        source.addComponent(b, Marker{});
        const std::string text = serialize(source);

        ECS::Registry copy;
        Report report;
        expect(load(copy, text, &report));

        expect(report.warnings.empty());
        expect(serialize(copy) == text);
    };

    "a loaded world can be saved and loaded again, any number of times"_test = [] {
        ECS::Registry first;
        load(first, worldText("[" + entity(A, R"({"sertest::Health":{"current":3}})") + "]"));
        const std::string text = serialize(first);
        ECS::Registry second;
        load(second, text);
        ECS::Registry third;
        load(third, serialize(second));
        expect(serialize(third) == text);
    };

    // ---- references

    "a reference is rebuilt to the entity created for its target"_test = [] {
        ECS::Registry world;
        const std::string link = R"({"sertest::Link":{"target":")" + B + R"(","others":[")" + A + R"(",")" + B + R"("]}})";
        expect(load(world, worldText("[" + entity(A, link) + "," + entity(B) + "]")));

        const ECS::Entity a = find(world, A);
        const ECS::Entity b = find(world, B);
        expect(world.getComponent<Link>(a).target == b);
        expect(world.getComponent<Link>(a).others.size() == std::size_t{2});
        expect(world.getComponent<Link>(a).others[0] == a);
        expect(world.getComponent<Link>(a).others[1] == b);
    };

    "a reference can point to an entity that comes later in the text"_test = [] {
        ECS::Registry world;
        const std::string link = R"({"sertest::Link":{"target":")" + C + R"("}})";
        expect(load(world, worldText("[" + entity(A, link) + "," + entity(B) + "," + entity(C) + "]")));
        expect(world.getComponent<Link>(find(world, A)).target == find(world, C));
    };

    "a reference to itself is rebuilt"_test = [] {
        ECS::Registry world;
        const std::string link = R"({"sertest::Link":{"target":")" + A + R"("}})";
        expect(load(world, worldText("[" + entity(A, link) + "]")));
        const ECS::Entity a = find(world, A);
        expect(world.getComponent<Link>(a).target == a);
    };

    "a reference written in upper case still finds its target"_test = [] {
        ECS::Registry world;
        std::string upper = A;
        for (char& c : upper)
            c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        const std::string link = R"({"sertest::Link":{"target":")" + upper + R"("}})";
        expect(load(world, worldText("[" + entity(A, link) + "]")));
        const ECS::Entity a = find(world, A);
        expect(world.getComponent<Link>(a).target == a);
    };

    "a null reference stays no entity"_test = [] {
        ECS::Registry world;
        Report report;
        expect(load(world, worldText("[" + entity(A, R"({"sertest::Link":{"target":null}})") + "]"), &report));
        expect(world.getComponent<Link>(find(world, A)).target.world() == ECS::InvalidWorld);
        expect(report.warnings.empty());
    };

    "a reference to an id that is not in the text is no entity, with a warning"_test = [] {
        ECS::Registry world;
        Report report;
        const std::string link = R"({"sertest::Link":{"target":")" + C + R"("}})";
        expect(load(world, worldText("[" + entity(A, link) + "]"), &report));
        expect(report.ok());
        expect(world.getComponent<Link>(find(world, A)).target.world() == ECS::InvalidWorld);
        expect(hasWarning(report, "sertest::Link.target"));
    };

    "a reference to an entity already in the world is not resolved"_test = [] {
        ECS::Registry world;
        const ECS::Entity existing = world.spawnEntity();
        world.addComponent(existing, Uuid{*uuid::fromString(C)});
        Report report;
        const std::string link = R"({"sertest::Link":{"target":")" + C + R"("}})";
        expect(load(world, worldText("[" + entity(A, link) + "]"), &report));
        expect(hasWarning(report, "sertest::Link.target"));
    };

    // ---- tolerance: warnings, the entity is still created

    "an unknown component is skipped with a warning"_test = [] {
        ECS::Registry world;
        Report report;
        expect(load(world, worldText("[" + entity(A, R"({"game::Nope":{"x":1},"sertest::Health":{"current":4}})") + "]"), &report));
        expect(report.ok());
        expect(hasWarning(report, "game::Nope"));
        expect(world.getComponent<Health>(find(world, A)).current == 4);
    };

    "a component that cannot be created is skipped with a warning"_test = [] {
        ECS::Registry world;
        Report report;
        expect(load(world, worldText("[" + entity(A, R"({"sertest::NoDefault":{"value":3}})") + "]"), &report));
        expect(report.ok());
        expect(hasWarning(report, "sertest::NoDefault"));
        expect(world.isAlive(find(world, A)));
    };

    "a Uuid among the components is ignored, the id of the entity wins"_test = [] {
        ECS::Registry world;
        Report report;
        const std::string other = R"({"Uuid":{"high":1,"low":2}})";
        expect(load(world, worldText("[" + entity(A, other) + "]"), &report));
        expect(hasWarning(report, "Uuid"));
        expect(world.getComponent<Uuid>(find(world, A)) == *uuid::fromString(A));
    };

    "a bad value in a component is a warning and keeps the default"_test = [] {
        ECS::Registry world;
        Report report;
        expect(load(world, worldText("[" + entity(A, R"({"sertest::Health":{"current":"x","max":5}})") + "]"), &report));
        expect(report.ok());
        expect(hasWarning(report, A + ":sertest::Health.current"));
        const Health& health = world.getComponent<Health>(find(world, A));
        expect(health.current == 100);
        expect(health.max == 5);
    };

    "a component that is not an object is a warning"_test = [] {
        ECS::Registry world;
        Report report;
        expect(load(world, worldText("[" + entity(A, R"({"sertest::Health":3})") + "]"), &report));
        expect(report.ok());
        expect(hasWarning(report, "sertest::Health"));
        expect(world.getComponent<Health>(find(world, A)).current == 100);
    };

    // ---- errors: nothing is created

    "text that is not JSON is refused"_test = [] {
        expectRefused("{not json");
        expectRefused("");
    };

    "text that is not a world is refused"_test = [] {
        expectRefused("[]", "metrov.world");
        expectRefused("{}", "metrov.world");
        expectRefused(R"({"format":"other","version":1,"entities":[]})", "metrov.world");
    };

    "a missing or malformed version is refused"_test = [] {
        expectRefused(R"({"format":"metrov.world","entities":[]})", "version");
        expectRefused(R"({"format":"metrov.world","version":"1","entities":[]})", "version");
    };

    "a version this build cannot read is refused"_test = [] {
        expectRefused(worldText("[]", serial::FormatVersion + 1), "unsupported");
        expectRefused(worldText("[]", 0), "unsupported");
    };

    "entities that is not an array is refused"_test = [] {
        expectRefused(worldText("{}"), "entities");
        expectRefused(R"({"format":"metrov.world","version":1})", "entities");
    };

    "an entity without a valid id is refused"_test = [] {
        expectRefused(worldText("[3]"), "id");
        expectRefused(worldText(R"([{"components":{}}])"), "id");
        expectRefused(worldText(R"([{"id":5}])"), "id");
        expectRefused(worldText(R"([{"id":"not-an-id"}])"), "not a valid id");
        expectRefused(worldText(R"([{"id":"00000000-0000-0000-0000-000000000000"}])"), "not a valid id");
    };

    "an id used twice in the text is refused"_test = [] {
        expectRefused(worldText("[" + entity(A) + "," + entity(A) + "]"), "twice");
    };

    "the same id in two cases is the same id"_test = [] {
        std::string upper = A;
        for (char& c : upper)
            c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        expectRefused(worldText("[" + entity(A) + "," + entity(upper) + "]"), "twice");
    };

    "components that is not an object is refused"_test = [] {
        expectRefused(worldText(R"([{"id":")" + A + R"(","components":[]}])"), "components");
    };

    "every wrong entity is reported, not only the first"_test = [] {
        ECS::Registry world;
        Report report;
        expect(!load(world, worldText(R"([{"id":"x"},{"id":"y"},)" + entity(A) + "]"), &report));
        expect(report.errors.size() == std::size_t{2});
        expect(world.entityCount() == std::size_t{0});
    };

    "one wrong entity refuses the whole text"_test = [] {
        ECS::Registry world;
        expect(!load(world, worldText("[" + entity(A) + R"(,{"id":"x"}])")));
        expect(world.entityCount() == std::size_t{0});
    };

    "an id already in the world is refused"_test = [] {
        ECS::Registry world;
        const ECS::Entity existing = world.spawnEntity();
        world.addComponent(existing, Uuid{*uuid::fromString(A)});
        Report report;
        expect(!load(world, worldText("[" + entity(B) + "," + entity(A) + "]"), &report));
        expect(hasError(report, "already in the world"));
        expect(world.entityCount() == std::size_t{1});
    };

    "loading the same text twice is refused the second time"_test = [] {
        ECS::Registry world;
        const std::string text = worldText("[" + entity(A) + "]");
        expect(load(world, text));
        Report report;
        expect(!load(world, text, &report));
        expect(hasError(report, "already in the world"));
        expect(world.entityCount() == std::size_t{1});
    };

    "load of a world works without a report"_test = [] {
        ECS::Registry world;
        expect(load(world, worldText("[]")));
        expect(!load(world, "{"));
    };

    // ---- a Report that already holds errors

    "load succeeds when the report already holds an error from an earlier call"_test = [] {
        ECS::Registry world;
        Report report;
        report.errors.push_back("from an earlier call");
        expect(load(world, worldText("[" + entity(A) + "]"), &report));
        expect(world.entityCount() == std::size_t{1});
        expect(report.errors.size() == std::size_t{1});   // kept, nothing added
    };

    "load still fails and adds its error to a report that already holds one"_test = [] {
        ECS::Registry world;
        Report report;
        report.errors.push_back("from an earlier call");
        expect(!load(world, "{not json", &report));
        expect(report.errors.size() == std::size_t{2});
        expect(world.entityCount() == std::size_t{0});
    };

    "a report holding warnings does not change the result of load"_test = [] {
        ECS::Registry world;
        Report report;
        report.warnings.push_back("earlier");
        expect(load(world, worldText("[" + entity(A) + "]"), &report));
        expect(report.warnings.size() == std::size_t{1});
    };

    "one report can follow several loads"_test = [] {
        ECS::Registry world;
        Report report;
        expect(load(world, worldText("[" + entity(A) + "]"), &report));
        expect(!load(world, worldText("[" + entity(A) + "]"), &report));   // A is already in the world
        expect(load(world, worldText("[" + entity(B) + "]"), &report));    // the error above does not fail this one
        expect(world.entityCount() == std::size_t{2});
        expect(report.errors.size() == std::size_t{1});
    };

    // ---- SceneInfo: the "systems" block

    "load gives the systems of the text, in order"_test = [] {
        ECS::Registry world;
        serial::SceneInfo scene;
        Report report;
        expect(load(world, sceneText(R"(["Input","Movement","Collision"])", "[" + entity(A) + "]"), &report, &scene));
        expect(report.ok());
        expect(scene.systems == std::vector<std::string>{"Input", "Movement", "Collision"});
        expect(world.entityCount() == std::size_t{1});
    };

    "load gives no systems for a text without a systems block"_test = [] {
        ECS::Registry world;
        serial::SceneInfo scene;
        scene.systems = {"Stale"};   // replaced, not appended to
        expect(load(world, worldText("[]"), nullptr, &scene));
        expect(scene.systems.empty());
    };

    "load gives an empty list for an empty systems block"_test = [] {
        ECS::Registry world;
        serial::SceneInfo scene;
        expect(load(world, sceneText("[]"), nullptr, &scene));
        expect(scene.systems.empty());
    };

    "load keeps duplicated system names"_test = [] {
        ECS::Registry world;
        serial::SceneInfo scene;
        expect(load(world, sceneText(R"(["Twin","Twin"])"), nullptr, &scene));
        expect(scene.systems == std::vector<std::string>{"Twin", "Twin"});
    };

    "load works without a SceneInfo, systems block or not"_test = [] {
        ECS::Registry world;
        expect(load(world, sceneText(R"(["Movement"])", "[" + entity(A) + "]")));
        expect(world.entityCount() == std::size_t{1});
    };

    "a systems block that is not an array is refused"_test = [] {
        expectRefused(sceneText(R"("Movement")", "[" + entity(A) + "]"), "'systems' must be an array");
        expectRefused(sceneText(R"({"Movement":true})"), "'systems' must be an array");
        expectRefused(sceneText("null"), "'systems' must be an array");
    };

    "a refused text leaves the SceneInfo untouched"_test = [] {
        ECS::Registry world;
        serial::SceneInfo scene;
        scene.systems = {"Before"};
        expect(!load(world, sceneText(R"(["Movement"])", R"([{"id":"not an id"}])"), nullptr, &scene));
        expect(scene.systems == std::vector<std::string>{"Before"});
    };

    "a system name that is not a non-empty string is skipped with a warning"_test = [] {
        ECS::Registry world;
        serial::SceneInfo scene;
        Report report;
        expect(load(world, sceneText(R"(["Input", 3, "", null, {"a":1}, "Movement"])"), &report, &scene));
        expect(scene.systems == std::vector<std::string>{"Input", "Movement"});
        expect(report.warnings.size() == std::size_t{4});
        expect(hasWarning(report, "systems[1]"));
        expect(hasWarning(report, "systems[2]"));
    };
}
