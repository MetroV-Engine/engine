// Levels: a scene written as a level file and read back (LevelFile), on an
// ECS world and on the demo scene, and the levels folder (LevelManager).

#include <boost/ut.hpp>

#include <algorithm>
#include <fstream>
#include <functional>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

#if !defined(_WIN32)
#include <csignal>
#include <sys/resource.h>
#endif

#include "ECS/ECS.hpp"
#include "ECS/Reflection/Reflection.hpp"
#include "Support/FreshWorkingDirectory.hpp"
#include "ecs/EcsScene/EcsScene.hpp"
#include "scene/DemoScene/DemoScene.hpp"
#include "scene/LevelFile/LevelFile.hpp"
#include "scene/LevelManager/LevelManager.hpp"

using namespace boost::ut;

// Prefixed with "Level": the TypeRegistry is process-wide.
namespace {
    struct LevelVector { float x = 0, y = 0; };
    struct LevelRgba { float r = 1, g = 1, b = 1, a = 1; };
    struct LevelMove { float speed = 1; int steps = 0; };
    enum class LevelTeam { Red, Blue };
    struct LevelThing
    {
        LevelVector position;
        LevelRgba color;
        LevelTeam team = LevelTeam::Red;
        bool visible = true;
        std::string label;
        LevelMove move;
        std::vector<int> path{1, 2};
    };
    struct LevelTag {};

    std::vector<std::string> rootNames(const HierarchyModel &scene)
    {
        std::vector<std::string> names;
        for (HierarchyModel::ObjectId id : scene.roots())
            names.push_back(scene.name(id));
        return names;
    }

    ECS::Entity entityNamed(const EcsScene &scene, const std::string &name)
    {
        for (HierarchyModel::ObjectId id : scene.roots())
            if (scene.name(id) == name)
                return *scene.entity(id);
        throw std::runtime_error("no object named " + name);
    }

    bool contains(const std::vector<std::string> &problems, const std::string &text)
    {
        return std::ranges::any_of(problems, [&](const std::string &p) { return p.find(text) != std::string::npos; });
    }

    bool refused(const std::function<void()> &change)
    {
        try {
            change();
        } catch (const std::runtime_error &) {
            return true;
        }
        return false;
    }
}

E_REFLECT(LevelVector) E_FIELD(x, float) E_FIELD(y, float) E_END
E_REFLECT(LevelRgba) E_FIELD(r, float) E_FIELD(g, float) E_FIELD(b, float) E_FIELD(a, float) E_END
E_REFLECT(LevelMove) E_FIELD(speed, float) E_FIELD(steps, int) E_END
E_REFLECT(LevelThing)
    E_FIELD(position, LevelVector)
    E_FIELD(color, LevelRgba)
    E_FIELD(team, LevelTeam)
    E_FIELD(visible, bool)
    E_FIELD(label, std::string)
    E_FIELD(move, LevelMove)
    E_FIELD(path, std::vector<int>)
E_END
E_REFLECT(LevelTag) E_END

void run_level_tests()
{
    "a level read back gives the same ECS world"_test = [] {
        ECS::Registry source;
        const ECS::Entity hero = source.spawnEntity("Hero");
        source.addComponent(hero, LevelThing{{12.5f, -3}, {0.1f, 0.2f, 0.3f, 0.4f}, LevelTeam::Blue, false, "say \"hi\"\nback\\slash",
                                      {2.75f, 7}, {9}});
        source.addComponent(hero, LevelTag{});
        source.spawnEntity("Empty");
        EcsScene sourceScene(source);
        const std::string text = LevelFile::write(sourceScene, sourceScene);

        ECS::Registry target;
        target.spawnEntity("Replaced");
        EcsScene targetScene(target);
        const std::vector<std::string> problems = LevelFile::read(text, targetScene, targetScene);

        expect(problems.empty());
        expect(LevelFile::write(targetScene, targetScene) == text) << "same level once read back";
        expect(rootNames(targetScene) == std::vector<std::string>{"Hero", "Empty"}) << "the previous objects are gone, the order is kept";
        const ECS::Entity read = entityNamed(targetScene, "Hero");
        const LevelThing &thing = target.getComponent<LevelThing>(read);
        expect(thing.position.x == 12.5f && thing.color.a == 0.4f && thing.team == LevelTeam::Blue && !thing.visible);
        expect(thing.label == std::string("say \"hi\"\nback\\slash")) << "quotes, new lines and backslashes";
        expect(thing.move.speed == 2.75f && thing.move.steps == 7) << "groups";
        expect(thing.path == std::vector<int>{1, 2}) << "read-only fields keep their default";
        expect(target.hasComponents<LevelTag>(read));
    };

    "a level read back gives the same demo scene, with its parents"_test = [] {
        DemoScene source(true);
        const std::string text = LevelFile::write(source, source);

        DemoScene target;
        const std::vector<std::string> problems = LevelFile::read(text, target, target);

        expect(problems.empty());
        expect(LevelFile::write(target, target) == text);
        expect(target.allObjects().size() == source.allObjects().size());
    };

    "an invalid level leaves the scene untouched"_test = [] {
        ECS::Registry world;
        world.spawnEntity("Kept");
        EcsScene scene(world);

        const std::vector<std::string> invalid{
            "not a level\n",
            "metrov-level 1\nobject 0 - \"A\"\nproperty \"x\" float 1\n",
            "metrov-level 1\nobject 0 - \"A\"\ncomponent \"LevelThing\"\nproperty \"visible\" bool maybe\n",
            "metrov-level 1\nobject 1 - \"A\"\n",
            "metrov-level 1\nobject 0 - \"A\n",
            "metrov-level 1\nobject 4294967296 - \"A\"\n",
        };
        for (const std::string &text : invalid) {
            bool thrown = false;
            try {
                LevelFile::read(text, scene, scene);
            } catch (const std::runtime_error &) {
                thrown = true;
            }
            expect(thrown) << text;
        }
        expect(rootNames(scene) == std::vector<std::string>{"Kept"});
    };

    "what the scene refuses is reported, the rest is read"_test = [] {
        ECS::Registry world;
        EcsScene scene(world);

        const std::vector<std::string> problems = LevelFile::read(
            "metrov-level 1\n"
            "object 0 - \"A\"\n"
            "component \"NoSuchComponent\"\n"
            "component \"LevelThing\"\n"
            "property \"unknown\" float 1\n"
            "property \"visible\" bool false\n"
            "object 1 0 \"Child\"\n",
            scene, scene);

        expect(contains(problems, "Cannot add NoSuchComponent")) << "unknown component";
        expect(contains(problems, "Cannot set LevelThing.unknown")) << "unknown property";
        expect(contains(problems, "flat")) << "the ECS has no parents yet";
        expect(rootNames(scene) == std::vector<std::string>{"A", "Child"});
        expect(!world.getComponent<LevelThing>(entityNamed(scene, "A")).visible);
    };

    "levels are created, saved, opened, renamed and deleted"_test = [] {
        FreshWorkingDirectory directory;
        ECS::Registry world;
        EcsScene scene(world);
        LevelManager levels(scene, scene);
        world.spawnEntity("Before");
        expect(levels.hasUnsavedChanges()) << "a scene that is in no level";

        levels.create("first");
        expect(rootNames(scene).empty()) << "a new level is empty";
        expect(levels.current() == std::string("first") && !levels.hasUnsavedChanges());
        world.spawnEntity("Hero");
        expect(levels.hasUnsavedChanges());
        levels.save();
        expect(!levels.hasUnsavedChanges());

        world.spawnEntity("Box");
        levels.saveAs("second");
        expect(levels.list() == std::vector<std::string>{"first", "second"});
        expect(levels.current() == std::string("second"));

        expect(levels.open("first").empty());
        expect(rootNames(scene) == std::vector<std::string>{"Hero"}) << "what was saved in first";
        expect(levels.current() == std::string("first") && !levels.hasUnsavedChanges());

        levels.rename("first", "renamed");
        expect(levels.list() == std::vector<std::string>{"renamed", "second"});
        expect(levels.current() == std::string("renamed")) << "the open level follows its file";

        levels.remove("second");
        levels.remove("renamed");
        expect(levels.list().empty());
        expect(levels.current().empty() && levels.hasUnsavedChanges()) << "the scene stays, in no level";
        expect(rootNames(scene) == std::vector<std::string>{"Hero"});
    };

    "refused level changes leave the levels and the scene as they were"_test = [] {
        FreshWorkingDirectory directory;
        ECS::Registry world;
        EcsScene scene(world);
        LevelManager levels(scene, scene);

        expect(refused([&] { levels.save(); })) << "no level open";
        for (const std::string &name : {"", "two words", "../outside", "dot.ted"})
            expect(refused([&] { levels.create(name); })) << name;
        levels.create("kept");
        world.spawnEntity("Unsaved");
        expect(refused([&] { levels.create("kept"); })) << "already exists";
        expect(refused([&] { levels.saveAs("kept"); })) << "already exists";
        expect(refused([&] { levels.open("missing"); }));
        expect(refused([&] { levels.rename("missing", "other"); }));
        expect(refused([&] { levels.remove("missing"); }));
        std::ofstream("levels/broken.level") << "not a level\n";
        expect(refused([&] { levels.open("broken"); }));

        expect(rootNames(scene) == std::vector<std::string>{"Unsaved"});
        expect(levels.current() == std::string("kept"));
        expect(levels.list() == std::vector<std::string>{"broken", "kept"});
    };

    "a level that cannot be created leaves the scene as it was"_test = [] {
        FreshWorkingDirectory directory;
        ECS::Registry world;
        world.spawnEntity("Unsaved");
        EcsScene scene(world);
        std::ofstream("levels") << "a file where the folder should be";
        LevelManager levels(scene, scene);

        expect(refused([&] { levels.create("new"); }));

        expect(rootNames(scene) == std::vector<std::string>{"Unsaved"});
        expect(levels.current().empty());
    };

#if !defined(_WIN32)
    "a save that fails halfway keeps the previous level file whole"_test = [] {
        FreshWorkingDirectory directory;
        DemoScene scene;
        scene.create(HierarchyModel::InvalidId, "Saved");
        LevelManager levels(scene, scene);
        levels.saveAs("first");
        const auto read = [] {
            std::ifstream file("levels/first.level", std::ios::binary);
            return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
        };
        const std::string before = read();
        scene.create(HierarchyModel::InvalidId, "Added");

        // Writes past 32 bytes fail, as on a full disk.
        rlimit previous{};
        getrlimit(RLIMIT_FSIZE, &previous);
        rlimit limited = previous;
        limited.rlim_cur = 32;
        const auto handler = std::signal(SIGXFSZ, SIG_IGN);
        setrlimit(RLIMIT_FSIZE, &limited);
        const bool failed = refused([&] { levels.save(); });
        setrlimit(RLIMIT_FSIZE, &previous);
        std::signal(SIGXFSZ, handler);

        expect(failed);
        expect(before.size() > 32_ul && read() == before) << "not cut at 32 bytes";
        expect(levels.hasUnsavedChanges()) << "the change is still to save";
    };
#endif
}
