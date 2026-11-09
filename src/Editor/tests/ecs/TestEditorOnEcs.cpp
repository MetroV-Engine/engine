// The real Editor on an ECS world, in an offscreen window: a script selects
// an entity (the Inspector then draws its components), plays, pauses, steps,
// stops and quits through the console commands, one action per frame.

#include <boost/ut.hpp>

#include <SDL3/SDL.h>

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "ECS/ECS.hpp"
#include "ECS/Reflection/Reflection.hpp"
#include "Support/Logs.hpp"
#include "core/CommandRegistry/CommandRegistry.hpp"
#include "core/Editor/Editor.hpp"
#include "core/GuiContext/GuiContext.hpp"
#include "ecs/EcsBackend/EcsBackend.hpp"

using namespace boost::ut;

// Prefixed with "Live": the TypeRegistry is process-wide.
namespace {
    struct LiveVector2 { float x = 0, y = 0; };
    struct LiveRgba { float r = 1, g = 1, b = 1, a = 1; };
    enum class LiveTeam { Red, Blue };
    struct LiveThing
    {
        LiveVector2 position;
        LiveVector2 velocity{100, 50};
        LiveRgba color;
        LiveTeam team = LiveTeam::Blue;
        std::uint8_t level = 1;
        std::vector<std::string> tags{"x", "y"};
        std::string label = "thing";
    };

    class MoveSystem : public ECS::ISystem
    {
    public:
        int updates = 0;
        void update(ECS::Registry &world, double dt) override
        {
            updates++;
            for (auto [entity, thing] : world.view<LiveThing>()) {
                (void)entity;
                thing.position.x += thing.velocity.x * static_cast<float>(dt);
                thing.position.y += thing.velocity.y * static_cast<float>(dt);
            }
            if (updates == 3)
                world.spawnEntity("spawned in play");
        }
        const std::string name() const override { return "MoveSystem"; }
    };

    // Frame number -> what to do on that frame (commands typed in the console).
    using Script = std::map<int, std::vector<std::string>>;

    // EcsBackend whose Viewport renderer, called once per frame, plays the
    // script: the test drives the editor like a user would.
    class ScriptedBackend : public SceneBackend
    {
    public:
        ScriptedBackend(GuiContext &gui, ECS::Registry &world, ECS::SystemManager &systems, Script script)
            : _inner(world, systems), _gui(gui), _script(std::move(script))
        {
        }

        HierarchyModel &hierarchy() override { return _inner.hierarchy(); }
        PropertyModel &properties() override { return _inner.properties(); }
        Simulation &simulation() override { return _inner.simulation(); }
        std::string description() const override { return _inner.description(); }
        void registerCommands(CommandRegistry &commands) override
        {
            _inner.registerCommands(commands);
            _commands = &commands;
        }
        Renderer renderer(const Selection &selection) override
        {
            // The Editor owns a non-const Selection: the script selects like
            // the Hierarchy panel does.
            Selection &editable = const_cast<Selection &>(selection);
            return [this, &editable](ImDrawList *, ImVec2, ImVec2) {
                step(editable);
                return ViewportView{};
            };
        }

        int frames = 0;
        HierarchyModel::ObjectId selected = HierarchyModel::InvalidId;

    private:
        EcsBackend _inner;
        GuiContext &_gui;
        Script _script;
        CommandRegistry *_commands = nullptr;

        void step(Selection &selection)
        {
            const auto actions = _script.find(++frames);
            if (actions == _script.end())
                return;
            for (const std::string &action : actions->second) {
                if (action == "select first") {
                    selected = hierarchy().roots().front();
                    selection.set(selected);
                } else if (action == "quit") {
                    _gui.stop();
                } else {
                    _commands->execute(action);
                }
            }
        }
    };

    // The editor saves its layout and shortcuts in the working directory:
    // each run gets a fresh one, removed afterwards.
    class FreshWorkingDirectory
    {
    public:
        FreshWorkingDirectory()
            : _previous(std::filesystem::current_path()),
              _path(std::filesystem::temp_directory_path()
                    / ("metrov-editor-test-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())))
        {
            std::filesystem::create_directories(_path);
            std::filesystem::current_path(_path);
        }
        ~FreshWorkingDirectory()
        {
            std::filesystem::current_path(_previous);
            std::error_code ignored;
            std::filesystem::remove_all(_path, ignored);
        }
        FreshWorkingDirectory(const FreshWorkingDirectory &) = delete;
        FreshWorkingDirectory &operator=(const FreshWorkingDirectory &) = delete;

    private:
        std::filesystem::path _previous;
        std::filesystem::path _path;
    };

    struct Run
    {
        int frames = 0;
        HierarchyModel::ObjectId selected = HierarchyModel::InvalidId;
    };

    // Opens the editor on world and runs script until it quits.
    Run runEditor(ECS::Registry &world, ECS::SystemManager &systems, Script script)
    {
        FreshWorkingDirectory directory;
        GuiContext gui("tests_editor", 1280, 720);
        auto backend = std::make_unique<ScriptedBackend>(gui, world, systems, std::move(script));
        ScriptedBackend &scripted = *backend;
        Run run;
        {
            Editor editor(gui, std::move(backend));
            editor.run();
            run.frames = scripted.frames;
            run.selected = scripted.selected;
        }
        return run;
    }
}

E_REFLECT(LiveVector2) E_FIELD(x, float) E_FIELD(y, float) E_END
E_REFLECT(LiveRgba) E_FIELD(r, float) E_FIELD(g, float) E_FIELD(b, float) E_FIELD(a, float) E_END
E_REFLECT(LiveThing)
    E_FIELD(position, LiveVector2)
    E_FIELD(velocity, LiveVector2)
    E_FIELD(color, LiveRgba)
    E_FIELD(team, LiveTeam)
    E_FIELD(level, std::uint8_t)
    E_FIELD(tags, std::vector<std::string>)
    E_FIELD(label, std::string)
E_END

void run_editor_on_ecs_tests()
{
    // No window on screen; SDL_VIDEO_DRIVER=x11 (or wayland...) shows it.
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");

    "the editor edits, plays and stops an ECS world, without error"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;
        MoveSystem &move = systems.addSystem<MoveSystem>(world);
        const ECS::Entity a = world.spawnEntity("A");
        world.addComponent(a, LiveThing{{10, 20}});
        const ECS::Entity b = world.spawnEntity();
        world.addComponent(b, LiveThing{{30, 40}});

        Logger::instance().clear();
        const Run run = runEditor(world, systems,
            {
                {3, {"select first", "objects", "components"}},
                {6, {"play"}},
                {20, {"pause"}},
                {22, {"step 2"}},
                {24, {"play"}},
                {30, {"stop"}},
                {34, {"quit"}},
            });

        expect(run.frames >= 34_i) << "the Viewport was drawn every frame";
        expect(world.isAlive(a) && world.isAlive(b));
        expect(run.selected != HierarchyModel::InvalidId);
        expect(move.updates > 3_i) << "Play ran the systems";
        expect(world.entityCount() == 2_ul) << "Stop killed the entity spawned in play";
        expect(world.getComponent<LiveThing>(a).position.x == 10.0f);
        expect(world.getComponent<LiveThing>(a).position.y == 20.0f);
        expect(world.getComponent<LiveThing>(b).position.x == 30.0f);
        expect(loggedErrors() == 0_ul);
    };

    "quitting during play restores the world too"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;
        MoveSystem &move = systems.addSystem<MoveSystem>(world);
        const ECS::Entity a = world.spawnEntity("A");
        world.addComponent(a, LiveThing{{10, 20}});

        Logger::instance().clear();
        runEditor(world, systems,
            {
                {3, {"select first"}},
                {5, {"play"}},
                {15, {"quit"}},
            });

        expect(move.updates > 3_i);
        expect(world.entityCount() == 1_ul);
        expect(world.getComponent<LiveThing>(a).position.x == 10.0f);
        expect(loggedErrors() == 0_ul);
    };
}
