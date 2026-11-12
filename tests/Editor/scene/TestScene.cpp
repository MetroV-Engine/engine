#include <boost/ut.hpp>

#include "Support/Headless.hpp"
#include "scene/DemoBackend.hpp"
#include "scene/DemoScene/DemoScene.hpp"
#include "scene/DemoSceneRenderer/DemoSceneRenderer.hpp"
#include "scene/DemoSimulation/DemoSimulation.hpp"
#include "scene/PlayController/PlayController.hpp"
#include "scene/Selection.hpp"

#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>

using namespace boost::ut;
using namespace EditorTest;

namespace {

// Records what the PlayController asks, and fails on demand.
struct FakeSimulation : Simulation
{
    std::vector<std::string> calls;
    double time = 0.0;
    bool failBegin = false;
    bool failUpdate = false;
    bool failEnd = false;

    void begin() override
    {
        if (failBegin)
            throw std::runtime_error("no level");
        calls.push_back("begin");
    }
    void update(double dt) override
    {
        if (failUpdate)
            throw std::runtime_error("division by zero");
        calls.push_back("update");
        time += dt;
    }
    void end() override
    {
        calls.push_back("end");
        if (failEnd)
            throw std::runtime_error("cannot restore");
    }
};

using ObjectId = HierarchyModel::ObjectId;

ObjectId find(const DemoScene &scene, const std::string &name)
{
    std::vector<ObjectId> stack = scene.roots();
    while (!stack.empty()) {
        const ObjectId id = stack.back();
        stack.pop_back();
        if (scene.name(id) == name)
            return id;
        for (ObjectId child : scene.children(id))
            stack.push_back(child);
    }
    return HierarchyModel::InvalidId;
}

} // namespace

void run_play_controller_tests()
{
    "play, pause, resume and stop, with the state changes reported"_test = [] {
        FakeSimulation simulation;
        PlayController play(simulation);
        std::vector<PlayState> states;
        play.setOnStateChanged([&](PlayState state) { states.push_back(state); });
        expect(play.isStopped());
        play.update(0.016);
        expect(simulation.calls.empty());

        play.play();
        play.play(); // Already playing
        play.update(0.016);
        play.pause();
        play.pause(); // Already paused
        play.update(0.016); // Paused: nothing runs
        play.play();
        play.stop();
        play.stop(); // Already stopped
        expect(simulation.calls == std::vector<std::string>{"begin", "update", "end"});
        expect(states == std::vector<PlayState>{PlayState::Playing, PlayState::Paused, PlayState::Playing, PlayState::Stopped});
    };

    "the time scale speeds the game up, clamped; a long frame is cut"_test = [] {
        FakeSimulation simulation;
        PlayController play(simulation);
        play.setTimeScale(2.0);
        play.play();
        play.update(0.01);
        expect(simulation.time == 0.02_d);
        play.update(1.0); // A hitch: at most MaxFrameDuration
        expect(std::abs(simulation.time - (0.02 + PlayController::MaxFrameDuration * 2.0)) < 1e-12);
        expect(play.getPlayTime() == simulation.time);
        expect(play.getFrameCount() == 2_ull);
        play.setTimeScale(100.0);
        expect(play.getTimeScale() == 10.0_d);
        play.setTimeScale(0.0);
        expect(play.getTimeScale() == 0.05_d);
    };

    "step: one frame then paused, starting the game if needed"_test = [] {
        FakeSimulation simulation;
        PlayController play(simulation);
        play.step();
        expect(play.getState() == PlayState::Paused);
        expect(play.getPlayTime() == PlayController::StepDuration);
        play.step();
        expect(play.getFrameCount() == 2_ull);
    };

    "a failing game: not started, paused on an error, stopped anyway"_test = [] {
        FakeSimulation simulation;
        PlayController play(simulation);
        LogWatch log;
        simulation.failBegin = true;
        play.play();
        expect(play.isStopped());
        play.step();
        expect(play.isStopped());
        expect(log.contains(LogLevel::Error, "Cannot start the game: no level"));

        simulation.failBegin = false;
        simulation.failUpdate = true;
        play.play();
        play.update(0.016);
        expect(play.getState() == PlayState::Paused);
        expect(log.contains(LogLevel::Error, "Game paused after an error: division by zero"));

        simulation.failEnd = true;
        play.stop();
        expect(play.isStopped());
        expect(log.contains(LogLevel::Error, "Error while stopping the game: cannot restore"));
    };

    "stateName"_test = [] {
        expect(std::string(PlayController::stateName(PlayState::Playing)) == "Playing");
        expect(std::string(PlayController::stateName(PlayState::Paused)) == "Paused");
        expect(std::string(PlayController::stateName(PlayState::Stopped)) == "Stopped");
    };
}

void run_demo_scene_tests()
{
    "an empty scene, or the demo level with its rooms and 250 collectibles"_test = [] {
        const DemoScene empty;
        expect(empty.roots().empty());
        const DemoScene level(true);
        expect(level.roots().size() == 4_u);
        const ObjectId collectibles = find(level, "Collectibles");
        expect(level.children(collectibles).size() == 250_u);
        expect(level.countDescendants(find(level, "Player")) == 3_u);
    };

    "hierarchy: create, rename, move and destroy, with the tree kept consistent"_test = [] {
        DemoScene scene;
        const ObjectId parent = scene.create(HierarchyModel::InvalidId, "Parent");
        const ObjectId child = scene.create(parent, "Child");
        const ObjectId grandchild = scene.create(child, "Grandchild");
        expect(scene.create(12345, "Orphan") == HierarchyModel::InvalidId);
        expect(scene.parent(child) == parent);
        expect(scene.parent(999) == HierarchyModel::InvalidId);
        expect(scene.children(999).empty());
        expect(scene.name(999).empty());
        expect(scene.isAncestor(parent, grandchild));
        expect(!scene.isAncestor(grandchild, parent));
        expect(scene.supportsParenting());
        expect(scene.idLabel(parent) == "Id " + std::to_string(parent));

        expect(scene.rename(child, "Renamed"));
        expect(!scene.rename(child, ""));
        expect(!scene.rename(999, "x"));
        expect(scene.name(child) == "Renamed");

        expect(!scene.setParent(parent, grandchild)); // Into its own descendant
        expect(!scene.setParent(parent, parent));
        expect(!scene.setParent(999, parent));
        expect(!scene.setParent(child, 999));
        expect(scene.setParent(child, parent)); // Already there
        expect(scene.setParent(grandchild, HierarchyModel::InvalidId));
        expect(scene.roots().size() == 2_u);

        expect(scene.destroy(parent));
        expect(!scene.exists(child));
        expect(!scene.destroy(parent));
        expect(scene.roots() == std::vector<ObjectId>{grandchild});
    };

    "properties: typed values, clamped to their range; read-only and unknown ones are refused"_test = [] {
        DemoScene scene;
        const ObjectId id = scene.create(HierarchyModel::InvalidId, "Box");
        expect(scene.components(id).size() == 1_u); // Transform
        expect(scene.components(999).empty());
        expect(!scene.removeComponent(id, "Transform")); // Core component

        expect(scene.addComponent(id, "Sprite"));
        expect(!scene.addComponent(id, "Sprite"));
        expect(!scene.addComponent(id, "Unknown"));
        expect(!scene.addComponent(999, "Sprite"));
        expect(scene.set(id, "Sprite", "layer", 500));
        expect(scene.get(id, "Sprite", "layer") == PropertyValue(100));
        expect(!scene.set(id, "Sprite", "layer", 1.5f)); // Wrong type
        expect(!scene.set(id, "Sprite", "nope", 1));
        expect(!scene.set(id, "Missing", "layer", 1));
        expect(scene.set(id, "Sprite", "tint", Color{0.5f, 0.5f, 0.5f, 1.0f}));
        expect(scene.set(id, "Sprite", "texture", std::string("hero.png")));
        expect(scene.set(id, "Sprite", "flipX", true));
        expect(std::holds_alternative<std::monostate>(scene.get(id, "Sprite", "nope")));
        expect(std::holds_alternative<std::monostate>(scene.get(id, "Missing", "layer")));

        expect(scene.set(id, "Transform", "rotation", 720.0f));
        expect(scene.get(id, "Transform", "rotation") == PropertyValue(360.0f));
        expect(scene.set(id, "Transform", "scale", Vec2{2, 3}));

        expect(scene.addComponent(id, "RigidBody"));
        expect(scene.set(id, "RigidBody", "bodyType", 9));
        expect(scene.get(id, "RigidBody", "bodyType") == PropertyValue(2)); // Last option

        expect(scene.addComponent(id, "Tilemap"));
        expect(scene.set(id, "Tilemap", "width", 10));
        expect(scene.get(id, "Tilemap", "tileCount") == PropertyValue(230));
        expect(!scene.set(id, "Tilemap", "tileCount", 5)); // Read-only

        expect(scene.addComponent(id, "Health"));
        expect(scene.set(id, "Health", "max", 3));
        expect(std::get<int>(scene.get(id, "Health", "current")) <= 3);

        expect(scene.addComponent(id, "Camera"));
        expect(scene.set(id, "Camera", "deadZone", Vec2{5000, -5}));
        expect(scene.get(id, "Camera", "deadZone") == PropertyValue(Vec2{1000, 0}));

        expect(scene.addComponent(id, "PlayerController"));
        expect(scene.set(id, "PlayerController", "movement.speed", 1.0f)); // In a group
        expect(!scene.set(id, "PlayerController", "movement", 1.0f));       // The group itself
        expect(!scene.set(id, "PlayerController", "movement.nope", 1.0f));
        expect(!scene.set(id, "Sprite", "layer.inner", 1));                 // Not a group

        const std::vector<std::string> addable = scene.addableComponents(id);
        expect(std::ranges::find(addable, "Sprite") == addable.end());
        expect(scene.addableComponents(999).empty());
        expect(scene.removeComponent(id, "Sprite"));
        expect(!scene.removeComponent(id, "Sprite"));
        expect(!scene.removeComponent(id, "Unknown"));
    };

    "DemoBackend: the scene, its game and its renderer"_test = [] {
        DemoBackend demo(true);
        DemoBackend empty(false);
        expect(demo.description() == "Editing the built-in demo level");
        expect(empty.description() == "Editing an empty built-in demo scene");
        expect(demo.hierarchy().roots().size() == 4_u);
        expect(demo.properties().components(demo.hierarchy().roots().front()).size() == 2_u); // Transform, Camera
        demo.simulation().begin();
        demo.simulation().update(1.0 / 60.0);
        demo.simulation().end();
        const Selection selection;
        expect(static_cast<bool>(demo.renderer(selection)));
    };
}

void run_demo_simulation_tests()
{
    "bodies fall to the ground, enemies patrol, collectibles float; Stop restores the level"_test = [] {
        DemoScene scene(true);
        DemoSimulation game(scene);
        const ObjectId player = find(scene, "Player");
        const ObjectId crawler = find(scene, "Crawler 1");
        const ObjectId shard = find(scene, "Soul Shard 1");
        const PropertyValue playerStart = scene.get(player, "Transform", "position");
        const PropertyValue crawlerStart = scene.get(crawler, "Transform", "position");
        const PropertyValue shardStart = scene.get(shard, "Transform", "position");

        game.begin();
        for (int i = 0; i < 240; i++)
            game.update(1.0 / 60.0);
        const Vec2 landed = std::get<Vec2>(scene.get(player, "Transform", "position"));
        // The player's box (28 high) rests on the ground.
        expect(landed.y == DemoSimulation::GroundY - 14.0f);
        expect(scene.get(crawler, "Transform", "position") != crawlerStart);
        expect(scene.get(shard, "Transform", "position") != shardStart);

        game.end();
        expect(scene.get(player, "Transform", "position") == playerStart);
        expect(scene.get(shard, "Transform", "position") == shardStart);
    };

    "the renderer: boxes fitted to the view, the selection highlighted, or a hint when empty"_test = [] {
        Gui gui;
        DemoScene level(true);
        Selection selection;
        selection.set(find(level, "Player"));
        const DemoSceneRenderer renderer(level, selection);
        DemoScene empty;
        const DemoSceneRenderer emptyRenderer(empty, selection);
        gui.frame([&] {
            ImDrawList *drawList = ImGui::GetForegroundDrawList();
            const int before = drawList->VtxBuffer.Size;
            renderer.draw(drawList, ImVec2(0, 0), ImVec2(800, 600));
            expect(drawList->VtxBuffer.Size > before);
            emptyRenderer.draw(drawList, ImVec2(0, 0), ImVec2(800, 600));
        });
    };
}
