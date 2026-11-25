#include <boost/ut.hpp>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include "Engine/Serialization/Serialization.hpp"
#include "Serialization/Support/Helpers.hpp"
#include "Serialization/Support/TestTypes.hpp"

using namespace boost::ut;
using serial::loadFile;
using serial::Report;
using serial::saveFile;
using serial::serialize;
using namespace sertest;

namespace {
    // A fresh directory, removed at the end of the test.
    class TempDir {
        public:
            TempDir() : _path(std::filesystem::temp_directory_path() / ("metrov_serial_" + uuid::toString(uuid::generate()))) {
                std::filesystem::create_directories(_path);
            }
            ~TempDir() {
                std::error_code ignored;
                std::filesystem::remove_all(_path, ignored);
            }
            const std::filesystem::path& path() const { return _path; }

        private:
            std::filesystem::path _path;
    };

    std::string readAll(const std::filesystem::path& path) {
        std::ifstream file(path, std::ios::binary);
        std::ostringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    void makeWorld(ECS::Registry& world) {
        const ECS::Entity entity = world.spawnEntity("saved");
        world.addComponent(entity, Uuid{uuid::generate()});
        world.addComponent(entity, Health{11, 22});
    }
}

void run_file_tests() {
    "a world saved to a file loads back identical"_test = [] {
        TempDir dir;
        ECS::Registry source;
        makeWorld(source);
        const auto file = dir.path() / "scene.json";

        Report saved;
        expect(saveFile(source, file, &saved));
        expect(saved.ok());
        expect(std::filesystem::exists(file));

        ECS::Registry copy;
        Report loaded;
        expect(loadFile(copy, file, &loaded));
        expect(loaded.ok());
        expect(serialize(copy) == serialize(source));
    };

    "the file contains exactly the serialized text"_test = [] {
        TempDir dir;
        ECS::Registry world;
        makeWorld(world);
        const auto file = dir.path() / "scene.json";
        saveFile(world, file);
        expect(readAll(file) == serialize(world));
    };

    "saving leaves no temporary file behind"_test = [] {
        TempDir dir;
        ECS::Registry world;
        makeWorld(world);
        saveFile(world, dir.path() / "scene.json");
        std::size_t files = 0;
        for (const auto& entry : std::filesystem::directory_iterator(dir.path())) {
            (void)entry;
            ++files;
        }
        expect(files == std::size_t{1});
    };

    "saving replaces an existing file"_test = [] {
        TempDir dir;
        const auto file = dir.path() / "scene.json";
        {
            std::ofstream old(file);
            old << "old content that is longer than nothing";
        }
        ECS::Registry world;
        makeWorld(world);
        expect(saveFile(world, file));
        expect(readAll(file) == serialize(world));
    };

    "an empty world can be saved and loaded"_test = [] {
        TempDir dir;
        ECS::Registry empty;
        const auto file = dir.path() / "empty.json";
        expect(saveFile(empty, file));
        ECS::Registry copy;
        expect(loadFile(copy, file));
        expect(copy.entityCount() == std::size_t{0});
    };

    "saving into a folder that does not exist is an error"_test = [] {
        TempDir dir;
        ECS::Registry world;
        makeWorld(world);
        const auto file = dir.path() / "missing" / "scene.json";
        Report report;
        expect(!saveFile(world, file, &report));
        expect(!report.ok());
        expect(!std::filesystem::exists(file));
    };

    "a failed save keeps the previous file"_test = [] {
        TempDir dir;
        const auto file = dir.path() / "scene.json";
        ECS::Registry good;
        makeWorld(good);
        saveFile(good, file);
        const std::string before = readAll(file);

        ECS::Registry bad;
        const ECS::Entity entity = bad.spawnEntity();
        Item item;
        item.id = "\xFF";
        bad.addComponent(entity, Item{item});
        Report report;
        expect(!saveFile(bad, file, &report));
        expect(readAll(file) == before);
    };

    "loading a file that does not exist is an error"_test = [] {
        TempDir dir;
        ECS::Registry world;
        Report report;
        expect(!loadFile(world, dir.path() / "nope.json", &report));
        expect(!report.ok());
        expect(world.entityCount() == std::size_t{0});
    };

    "loading a file that is not a world is an error and changes nothing"_test = [] {
        TempDir dir;
        const auto file = dir.path() / "garbage.json";
        {
            std::ofstream garbage(file);
            garbage << "this is not json";
        }
        ECS::Registry world;
        Report report;
        expect(!loadFile(world, file, &report));
        expect(!report.ok());
        expect(world.entityCount() == std::size_t{0});
    };

    "loading an empty file is an error"_test = [] {
        TempDir dir;
        const auto file = dir.path() / "empty.json";
        { std::ofstream empty(file); }
        ECS::Registry world;
        expect(!loadFile(world, file));
    };

    "saveFile and loadFile work without a report"_test = [] {
        TempDir dir;
        ECS::Registry world;
        makeWorld(world);
        const auto file = dir.path() / "scene.json";
        expect(saveFile(world, file));
        ECS::Registry copy;
        expect(loadFile(copy, file));
        expect(!loadFile(copy, dir.path() / "nope.json"));
    };

    // ---- a Report that already holds errors

    "saveFile writes the file when the report already holds an error"_test = [] {
        TempDir dir;
        ECS::Registry world;
        makeWorld(world);
        const auto file = dir.path() / "scene.json";
        Report report;
        report.errors.push_back("from an earlier call");
        expect(saveFile(world, file, &report));
        expect(readAll(file) == serialize(world));
        expect(report.errors.size() == std::size_t{1});
    };

    "saveFile still fails and adds its error to a report that already holds one"_test = [] {
        TempDir dir;
        ECS::Registry world;
        makeWorld(world);
        Report report;
        report.errors.push_back("from an earlier call");
        expect(!saveFile(world, dir.path() / "missing" / "scene.json", &report));
        expect(report.errors.size() == std::size_t{2});
    };

    "loadFile loads when the report already holds an error"_test = [] {
        TempDir dir;
        ECS::Registry source;
        makeWorld(source);
        const auto file = dir.path() / "scene.json";
        saveFile(source, file);

        ECS::Registry copy;
        Report report;
        report.errors.push_back("from an earlier call");
        expect(loadFile(copy, file, &report));
        expect(copy.entityCount() == std::size_t{1});
    };

    // ---- the temporary file

    "saving does not use a fixed temporary name"_test = [] {
        TempDir dir;
        const auto file = dir.path() / "scene.json";
        const auto fixed = dir.path() / "scene.json.tmp";
        {
            std::ofstream other(fixed);
            other << "belongs to someone else";
        }
        ECS::Registry world;
        makeWorld(world);
        expect(saveFile(world, file));
        expect(readAll(fixed) == "belongs to someone else");   // a second save at once would have overwritten it
    };

    "saving over something that cannot be replaced fails and leaves no temporary file"_test = [] {
        TempDir dir;
        const auto target = dir.path() / "scene.json";
        std::filesystem::create_directory(target);   // a folder where the file should go
        ECS::Registry world;
        makeWorld(world);
        Report report;
        expect(!saveFile(world, target, &report));
        expect(!report.ok());
        expect(std::filesystem::is_directory(target));
        std::size_t entries = 0;
        for (const auto& entry : std::filesystem::directory_iterator(dir.path())) {
            (void)entry;
            ++entries;
        }
        expect(entries == std::size_t{1});   // only the folder
    };

    "a file name without a folder is saved next to it"_test = [] {
        TempDir dir;
        const auto previous = std::filesystem::current_path();
        std::filesystem::current_path(dir.path());
        ECS::Registry world;
        makeWorld(world);
        const bool saved = saveFile(world, "relative.json");
        const bool exists = std::filesystem::exists(dir.path() / "relative.json");
        std::filesystem::current_path(previous);
        expect(saved);
        expect(exists);
    };

    // ---- assignMissingUuids through a file

    "saveFile with assignMissingUuids writes the same file every time"_test = [] {
        TempDir dir;
        ECS::Registry world;
        world.spawnEntity("scenery");
        serial::SaveOptions options;
        options.assignMissingUuids = true;

        expect(saveFile(world, dir.path() / "a.json", nullptr, options));
        expect(saveFile(world, dir.path() / "b.json", nullptr, options));

        expect(readAll(dir.path() / "a.json") == readAll(dir.path() / "b.json"));
    };

    // ---- SceneInfo through a file

    "the systems of a scene survive saveFile then loadFile"_test = [] {
        TempDir dir;
        ECS::Registry source;
        makeWorld(source);
        serial::SceneInfo saved;
        saved.systems = {"Input", "Movement"};
        expect(saveFile(source, dir.path() / "scene.json", nullptr, {}, &saved));

        ECS::Registry target;
        serial::SceneInfo loaded;
        expect(loadFile(target, dir.path() / "scene.json", nullptr, &loaded));

        expect(loaded.systems == saved.systems);
        expect(target.entityCount() == std::size_t{1});
    };

    "loadFile of a missing file leaves the SceneInfo untouched"_test = [] {
        TempDir dir;
        ECS::Registry world;
        serial::SceneInfo scene;
        scene.systems = {"Before"};
        expect(!loadFile(world, dir.path() / "missing.json", nullptr, &scene));
        expect(scene.systems == std::vector<std::string>{"Before"});
    };
}
