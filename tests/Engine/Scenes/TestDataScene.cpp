#include <boost/ut.hpp>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

#include "ECS/ECS.hpp"
#include "ECS/Reflection/Reflection.hpp"
#include "Engine/Components/Uuid.hpp"
#include "Engine/Engine.hpp"
#include "Engine/Scenes/DataScene.hpp"
#include "Engine/Scenes/SceneManager.hpp"
#include "Engine/Services/Services.hpp"

using namespace boost::ut;

// Test systems and components, in a namespace of their own: the catalog and
// the type registry are process-wide.
namespace dstest {
    struct Health {
        int hp = 10;
    };

    // A service, to check that systems see the Engine's services in init().
    struct Clock {
        int ticks = 0;
    };

    // What the test systems saw, reset by every test.
    struct Log {
        static inline std::vector<std::string> inits;     // "<system>:<entities seen>"
        static inline std::vector<std::string> runs;      // system names, in run order
        static inline bool clockSeenAtInit = false;

        static void clear() {
            inits.clear();
            runs.clear();
            clockSeenAtInit = false;
        }
    };

    class First : public ECS::ISystem {
        public:
            void init(ECS::Registry& world) override {
                Log::inits.push_back("dstest::First:" + std::to_string(world.entityCount()));
            }
            void update(ECS::Registry&, double) override { Log::runs.push_back("dstest::First"); }
            const std::string name() const override { return "dstest::First"; }
    };

    class Second : public ECS::ISystem {
        public:
            void init(ECS::Registry& world) override {
                Log::inits.push_back("dstest::Second:" + std::to_string(world.entityCount()));
            }
            void update(ECS::Registry&, double) override { Log::runs.push_back("dstest::Second"); }
            const std::string name() const override { return "dstest::Second"; }
    };

    class ClockProbe : public ECS::ISystem {
        public:
            void init(ECS::Registry& world) override { Log::clockSeenAtInit = hasService<Clock>(world); }
            void update(ECS::Registry&, double) override {}
            const std::string name() const override { return "dstest::ClockProbe"; }
    };

    class FailsToStart : public ECS::ISystem {
        public:
            void init(ECS::Registry&) override { throw std::runtime_error("no device"); }
            void update(ECS::Registry&, double) override {}
            const std::string name() const override { return "dstest::FailsToStart"; }
    };

    // Registered under "dstest::WrongName", but name() says otherwise: the catalog refuses it.
    class WrongName : public ECS::ISystem {
        public:
            void update(ECS::Registry&, double) override {}
            const std::string name() const override { return "SomethingElse"; }
    };

    // Added by a setup, not by the file.
    class FromCode : public ECS::ISystem {
        public:
            void update(ECS::Registry&, double) override { Log::runs.push_back("dstest::FromCode"); }
            const std::string name() const override { return "dstest::FromCode"; }
    };
}

E_REFLECT(dstest::Health)
    E_FIELD(hp, int)
E_END

E_SYSTEM(dstest::First)
E_SYSTEM(dstest::Second)
E_SYSTEM(dstest::ClockProbe)
E_SYSTEM(dstest::FailsToStart)
E_SYSTEM(dstest::WrongName)

namespace {
    const std::string A = "aaaaaaaa-aaaa-4aaa-8aaa-aaaaaaaaaaaa";
    const std::string B = "bbbbbbbb-bbbb-4bbb-8bbb-bbbbbbbbbbbb";

    std::string entity(const std::string& id, int hp) {
        return R"({"id":")" + id + R"(","components":{"dstest::Health":{"hp":)" + std::to_string(hp) + "}}}";
    }

    // A scene file text. systems: the "systems" JSON, or empty for no block.
    std::string sceneText(const std::string& systems, const std::string& entities = "[]") {
        std::string text = R"({"format":"metrov.world","version":1,)";
        if (!systems.empty())
            text += R"("systems":)" + systems + ",";
        return text + R"("entities":)" + entities + "}";
    }

    std::string twoEntities() {
        return "[" + entity(A, 3) + "," + entity(B, 4) + "]";
    }

    bool contains(const std::vector<std::string>& messages, const std::string& part) {
        for (const std::string& message : messages)
            if (message.find(part) != std::string::npos)
                return true;
        return false;
    }

    // A fresh directory, removed at the end of the test.
    class TempDir {
        public:
            TempDir() : _path(std::filesystem::temp_directory_path() / ("metrov_datascene_" + uuid::toString(uuid::generate()))) {
                std::filesystem::create_directories(_path);
            }
            ~TempDir() {
                std::error_code ignored;
                std::filesystem::remove_all(_path, ignored);
            }
            std::filesystem::path path(const std::string& file) const { return _path / file; }

            std::filesystem::path write(const std::string& file, const std::string& text) const {
                std::ofstream(path(file), std::ios::binary) << text;
                return path(file);
            }

        private:
            std::filesystem::path _path;
    };
}

void run_data_scene_tests() {
    // ---- fromText

    "fromText loads the entities and the list of systems, without creating the systems"_test = [] {
        dstest::Log::clear();
        serial::Report report;
        auto scene = DataScene::fromText(sceneText(R"(["dstest::First","dstest::Second"])", twoEntities()), "level", &report);

        expect(scene != nullptr);
        expect(report.ok());
        expect(scene->name() == std::string("level"));
        expect(scene->registry().entityCount() == std::size_t{2});
        expect(scene->info().systems == std::vector<std::string>{"dstest::First", "dstest::Second"});
        expect(scene->systems().size() == std::size_t{0});   // created in onEnter()
        expect(dstest::Log::inits.empty());
    };

    "fromText gives the scene a default name"_test = [] {
        auto scene = DataScene::fromText(sceneText(""));
        expect(scene != nullptr);
        expect(scene->name() == std::string("DataScene"));
    };

    "fromText returns nullptr for a text that cannot be loaded, with its errors"_test = [] {
        serial::Report report;
        expect(DataScene::fromText("{not json", "broken", &report) == nullptr);
        expect(!report.ok());
        expect(DataScene::fromText("", "empty") == nullptr);
    };

    "fromText returns nullptr for a systems block that is not an array"_test = [] {
        serial::Report report;
        expect(DataScene::fromText(sceneText(R"("dstest::First")"), "level", &report) == nullptr);
        expect(contains(report.errors, "'systems' must be an array"));
    };

    "fromText keeps the warnings of the load in loadReport and in the caller's report"_test = [] {
        serial::Report report;
        const std::string unknown = R"([{"id":")" + A + R"(","components":{"NoSuchComponent":{}}}])";
        auto scene = DataScene::fromText(sceneText("", unknown), "level", &report);

        expect(scene != nullptr);
        expect(contains(scene->loadReport().warnings, "unknown component"));
        expect(contains(report.warnings, "unknown component"));
    };

    "fromText appends to the caller's report without clearing it"_test = [] {
        serial::Report report;
        report.warnings.push_back("earlier");
        expect(DataScene::fromText(sceneText(""), "level", &report) != nullptr);
        expect(report.warnings == std::vector<std::string>{"earlier"});
    };

    // ---- onEnter: the systems of the file

    "onEnter creates the systems of the file, in order, after the entities"_test = [] {
        dstest::Log::clear();
        auto scene = DataScene::fromText(sceneText(R"(["dstest::Second","dstest::First"])", twoEntities()));

        scene->onEnter();
        scene->update(0.0);

        expect(scene->systems().size() == std::size_t{2});
        expect(dstest::Log::inits == std::vector<std::string>{"dstest::Second:2", "dstest::First:2"});
        expect(dstest::Log::runs == std::vector<std::string>{"dstest::Second", "dstest::First"});
        expect(scene->loadReport().ok());
    };

    "onEnter creates the systems once, even when called again"_test = [] {
        dstest::Log::clear();
        auto scene = DataScene::fromText(sceneText(R"(["dstest::First"])"));
        scene->onEnter();
        scene->onEnter();
        expect(scene->systems().size() == std::size_t{1});
        expect(dstest::Log::inits.size() == std::size_t{1});
    };

    "a scene without a systems block has no systems"_test = [] {
        auto scene = DataScene::fromText(sceneText("", twoEntities()));
        scene->onEnter();
        expect(scene->systems().size() == std::size_t{0});
        expect(scene->info().systems.empty());
    };

    "a system listed twice runs twice"_test = [] {
        dstest::Log::clear();
        auto scene = DataScene::fromText(sceneText(R"(["dstest::First","dstest::First"])"));
        scene->onEnter();
        scene->update(0.0);
        expect(dstest::Log::runs == std::vector<std::string>{"dstest::First", "dstest::First"});
    };

    "an unknown system is a warning, and the other systems are created"_test = [] {
        dstest::Log::clear();
        auto scene = DataScene::fromText(sceneText(R"(["dstest::First","NoSuchSystem","dstest::Second"])"));
        scene->onEnter();

        expect(scene->systems().size() == std::size_t{2});
        expect(scene->loadReport().ok());
        expect(contains(scene->loadReport().warnings, "unknown system 'NoSuchSystem'"));
    };

    "a system the catalog refuses (name() differs) is a warning"_test = [] {
        auto scene = DataScene::fromText(sceneText(R"(["dstest::WrongName","dstest::First"])"));
        scene->onEnter();

        expect(scene->systems().size() == std::size_t{1});
        expect(contains(scene->loadReport().warnings, "'dstest::WrongName' cannot be created"));
    };

    "a system whose init throws is an error, and the other systems are created"_test = [] {
        dstest::Log::clear();
        auto scene = DataScene::fromText(sceneText(R"(["dstest::FailsToStart","dstest::First"])"));
        scene->onEnter();

        expect(scene->systems().size() == std::size_t{1});
        expect(!scene->loadReport().ok());
        expect(contains(scene->loadReport().errors, "'dstest::FailsToStart' failed to start: no device"));
    };

    // ---- setup

    "the setup runs in onEnter, after the systems of the file"_test = [] {
        dstest::Log::clear();
        auto scene = DataScene::fromText(sceneText(R"(["dstest::First"])"));
        bool sawFileSystems = false;
        scene->setSetup([&](DataScene& self) {
            sawFileSystems = self.systems().size() == 1;
            self.systems().addSystem<dstest::FromCode>(self.registry());
        });

        scene->onEnter();
        scene->update(0.0);

        expect(sawFileSystems);
        expect(dstest::Log::runs == std::vector<std::string>{"dstest::First", "dstest::FromCode"});
    };

    "the setup runs once, and an empty setup does nothing"_test = [] {
        auto scene = DataScene::fromText(sceneText(""));
        int calls = 0;
        scene->setSetup([&](DataScene&) { ++calls; });
        scene->onEnter();
        scene->onEnter();
        expect(calls == 1);

        auto plain = DataScene::fromText(sceneText(""));
        plain->setSetup({});
        plain->onEnter();
        expect(plain->systems().size() == std::size_t{0});
    };

    "a setup that throws a std::exception is an error, and the file's systems are still created"_test = [] {
        dstest::Log::clear();
        auto scene = DataScene::fromText(sceneText(R"(["dstest::First"])"));
        scene->setSetup([](DataScene&) { throw std::runtime_error("boom"); });

        expect(nothrow([&] { scene->onEnter(); }));

        expect(scene->systems().names() == std::vector<std::string>{"dstest::First"});
        expect(contains(scene->loadReport().errors, "setup failed: boom"));
    };

    "a setup that throws something else than a std::exception is an error too"_test = [] {
        auto scene = DataScene::fromText(sceneText(R"(["dstest::First"])"));
        scene->setSetup([](DataScene&) { throw 42; });

        expect(nothrow([&] { scene->onEnter(); }));

        expect(scene->systems().size() == std::size_t{1});
        expect(contains(scene->loadReport().errors, "setup failed: unknown error"));
    };

    "a setup that throws after adding a system keeps that system"_test = [] {
        auto scene = DataScene::fromText(sceneText(""));
        scene->setSetup([](DataScene& self) {
            self.systems().addSystem<dstest::FromCode>(self.registry());
            throw std::runtime_error("late");
        });
        scene->onEnter();
        expect(scene->systems().names() == std::vector<std::string>{"dstest::FromCode"});
        expect(contains(scene->loadReport().errors, "setup failed: late"));
    };

    // ---- setReportHandler

    "the report handler is called once, after onEnter, with the whole report"_test = [] {
        const std::string unknown = R"([{"id":")" + A + R"(","components":{"NoSuchComponent":{}}}])";
        auto scene = DataScene::fromText(sceneText(R"(["NoSuchSystem","dstest::First"])", unknown), "level");
        scene->setSetup([](DataScene&) { throw std::runtime_error("boom"); });
        int calls = 0;
        std::string seenId;
        serial::Report seen;
        scene->setReportHandler([&](const std::string& id, const serial::Report& report) {
            ++calls;
            seenId = id;
            seen = report;
        });

        expect(calls == 0);   // not before onEnter
        scene->onEnter();
        scene->onEnter();

        expect(calls == 1);
        expect(seenId == std::string("level"));
        expect(contains(seen.warnings, "unknown component"));      // the load
        expect(contains(seen.warnings, "unknown system"));         // the systems
        expect(contains(seen.errors, "setup failed: boom"));       // the setup
    };

    "the report handler is called even when everything went well"_test = [] {
        auto scene = DataScene::fromText(sceneText(R"(["dstest::First"])"), "level");
        bool called = false;
        bool ok = false;
        scene->setReportHandler([&](const std::string&, const serial::Report& report) {
            called = true;
            ok = report.ok() && report.warnings.empty();
        });
        scene->onEnter();
        expect(called);
        expect(ok);
    };

    "a report handler that throws does not leave onEnter, and its error is recorded"_test = [] {
        auto scene = DataScene::fromText(sceneText(""), "level");
        scene->setReportHandler([](const std::string&, const serial::Report&) { throw std::runtime_error("bad handler"); });
        expect(nothrow([&] { scene->onEnter(); }));
        expect(contains(scene->loadReport().errors, "report handler failed: bad handler"));

        auto other = DataScene::fromText(sceneText(""), "level");
        other->setReportHandler([](const std::string&, const serial::Report&) { throw 7; });
        expect(nothrow([&] { other->onEnter(); }));
        expect(contains(other->loadReport().errors, "report handler failed: unknown error"));
    };

    // ---- setSystems

    "setSystems replaces the systems that save writes and the next load creates"_test = [] {
        dstest::Log::clear();
        TempDir dir;
        auto scene = DataScene::fromText(sceneText(R"(["dstest::First"])"), "level");

        scene->setSystems({"dstest::Second", "dstest::First"});   // add and reorder
        expect(scene->info().systems == std::vector<std::string>{"dstest::Second", "dstest::First"});
        expect(scene->save(dir.path("saved.json")));

        auto reloaded = DataScene::fromFile(dir.path("saved.json"));
        reloaded->onEnter();
        expect(reloaded->systems().names() == std::vector<std::string>{"dstest::Second", "dstest::First"});
    };

    "setSystems removes a system from the data"_test = [] {
        auto scene = DataScene::fromText(sceneText(R"(["dstest::First","dstest::Second"])"), "level");
        auto systems = scene->info().systems;
        systems.erase(systems.begin());
        scene->setSystems(systems);
        expect(scene->info().systems == std::vector<std::string>{"dstest::Second"});

        scene->setSystems({});
        expect(scene->info().systems.empty());
    };

    "setSystems called before onEnter changes which systems onEnter creates"_test = [] {
        dstest::Log::clear();
        auto scene = DataScene::fromText(sceneText(R"(["dstest::First"])"), "level");

        scene->setSystems({"dstest::Second"});
        scene->onEnter();

        expect(scene->systems().names() == std::vector<std::string>{"dstest::Second"});
        expect(dstest::Log::inits == std::vector<std::string>{"dstest::Second:0"});
    };

    "setSystems does not touch the systems already running"_test = [] {
        auto scene = DataScene::fromText(sceneText(R"(["dstest::First"])"), "level");
        scene->onEnter();
        scene->setSystems({"dstest::Second"});
        expect(scene->systems().names() == std::vector<std::string>{"dstest::First"});
        expect(scene->info().systems == std::vector<std::string>{"dstest::Second"});
    };

    "setSystems skips empty names with a warning in the given report, and keeps unknown names"_test = [] {
        auto scene = DataScene::fromText(sceneText(""), "level");
        serial::Report report;
        scene->setSystems({"dstest::First", "", "NoSuchSystem", ""}, &report);
        expect(scene->info().systems == std::vector<std::string>{"dstest::First", "NoSuchSystem"});
        expect(report.warnings.size() == std::size_t{2});
        expect(contains(report.warnings, "setSystems: systems[1]"));
    };

    "setSystems does not write to loadReport, which describes the load"_test = [] {
        auto scene = DataScene::fromText(sceneText(""), "level");
        scene->setSystems({""});            // a warning, but no report given
        expect(scene->loadReport().warnings.empty());
        expect(scene->info().systems.empty());
    };

    "setSystems appends to the given report without clearing it, and adds nothing when all names are valid"_test = [] {
        auto scene = DataScene::fromText(sceneText(""), "level");
        serial::Report report;
        report.errors.push_back("earlier");
        scene->setSystems({"dstest::First"}, &report);
        expect(report.errors == std::vector<std::string>{"earlier"});
        expect(report.warnings.empty());
    };

    // ---- fromFile

    "fromFile loads a scene file, named after the file"_test = [] {
        TempDir dir;
        const auto file = dir.write("level1.json", sceneText(R"(["dstest::First"])", twoEntities()));

        auto scene = DataScene::fromFile(file);

        expect(scene != nullptr);
        expect(scene->name() == std::string("level1"));
        expect(scene->registry().entityCount() == std::size_t{2});
        expect(scene->info().systems == std::vector<std::string>{"dstest::First"});
    };

    "fromFile uses the name it is given"_test = [] {
        TempDir dir;
        const auto file = dir.write("level1.json", sceneText(""));
        auto scene = DataScene::fromFile(file, "intro");
        expect(scene->name() == std::string("intro"));
    };

    "fromFile returns nullptr for a missing file, with an error"_test = [] {
        TempDir dir;
        serial::Report report;
        expect(DataScene::fromFile(dir.path("missing.json"), {}, &report) == nullptr);
        expect(contains(report.errors, "cannot open"));
        expect(contains(report.errors, "missing.json"));
    };

    "fromFile returns nullptr for a file that is not a scene"_test = [] {
        TempDir dir;
        const auto file = dir.write("broken.json", R"({"format":"something.else"})");
        serial::Report report;
        expect(DataScene::fromFile(file, {}, &report) == nullptr);
        expect(!report.ok());
    };

    // ---- save

    "save writes a file that fromFile loads back: entities, values and systems"_test = [] {
        TempDir dir;
        auto scene = DataScene::fromText(sceneText(R"(["dstest::First","dstest::Second"])", twoEntities()), "level");
        expect(scene->save(dir.path("saved.json")));

        auto reloaded = DataScene::fromFile(dir.path("saved.json"));

        expect(reloaded != nullptr);
        expect(reloaded->registry().entityCount() == std::size_t{2});
        expect(reloaded->info().systems == std::vector<std::string>{"dstest::First", "dstest::Second"});
        int total = 0;
        for (auto [entity, health] : reloaded->registry().view<dstest::Health>()) {
            (void)entity;
            total += health.hp;
        }
        expect(total == 7);   // 3 + 4
    };

    "save writes only the systems of the data, so reloading creates no duplicate"_test = [] {
        dstest::Log::clear();
        TempDir dir;
        const auto addFromCode = [](DataScene& self) { self.systems().addSystem<dstest::FromCode>(self.registry()); };
        auto scene = DataScene::fromText(sceneText(R"(["dstest::First"])"), "level");
        scene->setSetup(addFromCode);
        scene->onEnter();
        expect(scene->systems().names() == std::vector<std::string>{"dstest::First", "dstest::FromCode"});

        expect(scene->save(dir.path("saved.json")));
        auto reloaded = DataScene::fromFile(dir.path("saved.json"));
        reloaded->setSetup(addFromCode);
        reloaded->onEnter();

        expect(reloaded->info().systems == std::vector<std::string>{"dstest::First"});
        expect(reloaded->systems().names() == std::vector<std::string>{"dstest::First", "dstest::FromCode"});
    };

    "save gives a Uuid to entities without one, so saving twice writes the same file"_test = [] {
        TempDir dir;
        auto scene = DataScene::fromText(sceneText(""), "level");
        const ECS::Entity added = scene->registry().spawnEntity();   // made by code: no Uuid yet
        scene->registry().addComponent(added, dstest::Health{9});

        expect(scene->save(dir.path("a.json")));
        expect(scene->save(dir.path("b.json")));

        expect(scene->registry().hasComponents<Uuid>(added));
        std::ifstream a(dir.path("a.json")), b(dir.path("b.json"));
        const std::string textA((std::istreambuf_iterator<char>(a)), std::istreambuf_iterator<char>());
        const std::string textB((std::istreambuf_iterator<char>(b)), std::istreambuf_iterator<char>());
        expect(textA == textB);
    };

    "save of a scene without a systems block writes an empty one"_test = [] {
        TempDir dir;
        auto scene = DataScene::fromText(sceneText("", twoEntities()), "level");
        expect(scene->save(dir.path("saved.json")));
        auto reloaded = DataScene::fromFile(dir.path("saved.json"));
        expect(reloaded != nullptr);
        expect(reloaded->info().systems.empty());
    };

    "save into a folder that does not exist fails, with an error, and writes nothing"_test = [] {
        TempDir dir;
        auto scene = DataScene::fromText(sceneText("", twoEntities()), "level");
        serial::Report report;
        report.warnings.push_back("earlier");

        expect(!scene->save(dir.path("no/such/folder/saved.json"), &report));

        expect(!report.ok());
        expect(report.warnings.front() == std::string("earlier"));   // appended to, not cleared
        expect(!std::filesystem::exists(dir.path("no/such/folder/saved.json")));
    };

    // ---- addDataScene: through the scene registry

    "addDataScene: push(id) builds the scene from its file and creates its systems"_test = [] {
        dstest::Log::clear();
        TempDir dir;
        const auto file = dir.write("level1.json", sceneText(R"(["dstest::First"])", twoEntities()));
        SceneManager scenes;
        expect(addDataScene(scenes.registry(), "level1", file));

        expect(scenes.push("level1"));

        Scene* top = scenes.top();
        expect(top != nullptr);
        expect(top->name() == std::string("level1"));
        expect(top->registry().entityCount() == std::size_t{2});
        expect(top->systems().size() == std::size_t{1});   // onEnter ran
        expect(dstest::Log::inits == std::vector<std::string>{"dstest::First:2"});
    };

    "addDataScene: a file that cannot be loaded makes push(id) fail and push nothing"_test = [] {
        TempDir dir;
        const auto broken = dir.write("broken.json", "{not json");
        SceneManager scenes;
        expect(addDataScene(scenes.registry(), "broken", broken));
        expect(addDataScene(scenes.registry(), "missing", dir.path("missing.json")));

        expect(!scenes.push("broken"));
        expect(!scenes.push("missing"));
        expect(scenes.empty());
    };

    "addDataScene refuses an empty or already taken id"_test = [] {
        TempDir dir;
        const auto file = dir.write("level1.json", sceneText(""));
        SceneManager scenes;
        expect(!addDataScene(scenes.registry(), "", file));
        expect(addDataScene(scenes.registry(), "level1", file));
        expect(!addDataScene(scenes.registry(), "level1", file));
    };

    "addDataScene: the setup runs after the systems of the file"_test = [] {
        dstest::Log::clear();
        TempDir dir;
        const auto file = dir.write("level1.json", sceneText(R"(["dstest::First"])"));
        SceneManager scenes;
        addDataScene(scenes.registry(), "level1", file, [](DataScene& scene) {
            scene.systems().addSystem<dstest::FromCode>(scene.registry());
        });

        scenes.push("level1");
        scenes.update(0.0);

        expect(dstest::Log::runs == std::vector<std::string>{"dstest::First", "dstest::FromCode"});
    };

    "addDataScene: push(id) does not throw when the setup throws, and the scene is pushed"_test = [] {
        TempDir dir;
        const auto file = dir.write("level1.json", sceneText(R"(["dstest::First"])"));
        SceneManager scenes;
        addDataScene(scenes.registry(), "level1", file, [](DataScene&) { throw std::runtime_error("boom"); });

        bool pushed = false;
        expect(nothrow([&] { pushed = scenes.push("level1"); }));

        expect(pushed);
        expect(scenes.top() != nullptr);
        expect(scenes.top()->systems().size() == std::size_t{1});
        auto* scene = dynamic_cast<DataScene*>(scenes.top());
        expect(scene != nullptr);
        expect(contains(scene->loadReport().errors, "setup failed: boom"));
    };

    "addDataScene: onReport gets the whole report after push(id), once"_test = [] {
        TempDir dir;
        const auto file = dir.write("level1.json", sceneText(R"(["dstest::First","NoSuchSystem"])"));
        SceneManager scenes;
        int calls = 0;
        std::string seenId;
        serial::Report seen;
        addDataScene(scenes.registry(), "level1", file, {}, [&](const std::string& id, const serial::Report& report) {
            ++calls;
            seenId = id;
            seen = report;
        });

        expect(scenes.push("level1"));

        expect(calls == 1);
        expect(seenId == std::string("level1"));
        expect(seen.ok());
        expect(contains(seen.warnings, "unknown system 'NoSuchSystem'"));
    };

    "addDataScene: onReport gets the load errors when push(id) fails"_test = [] {
        TempDir dir;
        const auto broken = dir.write("broken.json", "{not json");
        SceneManager scenes;
        int calls = 0;
        serial::Report seen;
        const auto handler = [&](const std::string&, const serial::Report& report) {
            ++calls;
            seen = report;
        };
        addDataScene(scenes.registry(), "broken", broken, {}, handler);
        addDataScene(scenes.registry(), "missing", dir.path("missing.json"), {}, handler);

        expect(!scenes.push("broken"));
        expect(calls == 1);
        expect(!seen.ok());

        expect(!scenes.push("missing"));
        expect(calls == 2);
        expect(contains(seen.errors, "cannot open"));
        expect(scenes.empty());
    };

    "addDataScene: onReport is called on every push(id)"_test = [] {
        TempDir dir;
        const auto file = dir.write("level1.json", sceneText(""));
        SceneManager scenes;
        int calls = 0;
        addDataScene(scenes.registry(), "level1", file, {}, [&](const std::string&, const serial::Report&) { ++calls; });

        scenes.push("level1");
        scenes.pop();
        scenes.push("level1");

        expect(calls == 2);
    };

    "addDataScene: an onReport that throws does not leave push(id)"_test = [] {
        TempDir dir;
        const auto file = dir.write("level1.json", sceneText(""));
        const auto broken = dir.write("broken.json", "{not json");
        SceneManager scenes;
        const auto throwing = [](const std::string&, const serial::Report&) { throw std::runtime_error("bad"); };
        addDataScene(scenes.registry(), "level1", file, {}, throwing);
        addDataScene(scenes.registry(), "broken", broken, {}, throwing);

        bool pushed = false;
        expect(nothrow([&] { pushed = scenes.push("level1"); }));
        expect(pushed);
        expect(nothrow([&] { pushed = scenes.push("broken"); }));
        expect(!pushed);
    };

    "addDataScene: popping then pushing the scene again reads the file again"_test = [] {
        TempDir dir;
        const auto file = dir.write("level1.json", sceneText("", "[" + entity(A, 3) + "]"));
        SceneManager scenes;
        addDataScene(scenes.registry(), "level1", file);

        scenes.push("level1");
        expect(scenes.top()->registry().entityCount() == std::size_t{1});
        scenes.pop();

        dir.write("level1.json", sceneText("", twoEntities()));
        scenes.push("level1");
        expect(scenes.top()->registry().entityCount() == std::size_t{2});
    };

    "addDataScene: the systems see the Engine's services in their init"_test = [] {
        dstest::Log::clear();
        TempDir dir;
        const auto file = dir.write("level1.json", sceneText(R"(["dstest::ClockProbe"])"));
        Engine engine;
        engine.init();
        engine.addService<dstest::Clock>();
        addDataScene(engine.scenes().registry(), "level1", file);

        expect(engine.scenes().push("level1"));

        expect(dstest::Log::clockSeenAtInit);
        engine.shutdown();
    };
}
