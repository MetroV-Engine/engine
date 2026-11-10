// Levels: a scene written as a level file and read back (LevelFile), on an
// ECS world and on the demo scene.

#include <boost/ut.hpp>

#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

#include "ECS/ECS.hpp"
#include "ECS/Reflection/Reflection.hpp"
#include "ecs/EcsScene/EcsScene.hpp"
#include "scene/DemoScene/DemoScene.hpp"
#include "scene/LevelFile/LevelFile.hpp"

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
}
