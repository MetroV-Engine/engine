// The real Editor on an ECS world, in an offscreen window: a script selects
// an entity (the Inspector then draws its components), plays, pauses, steps,
// stops and quits through the console commands, one action per frame. The
// panels are measured on screen and the saved layout read back.

#include <boost/ut.hpp>

#include <SDL3/SDL.h>

#include <imgui.h>
#include <imgui_internal.h>

#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iterator>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "ECS/ECS.hpp"
#include "ECS/Reflection/Reflection.hpp"
#include "Support/FreshWorkingDirectory.hpp"
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

    // How far the Hierarchy's tree is scrolled, out of how far it can be.
    struct TreeScroll
    {
        float position = 0.0f;
        float max = 0.0f;
    };

    // Where a panel is on screen ("measure panels" in a script).
    struct PanelPlace
    {
        bool docked = false;
        ImVec2 pos;
    };

    // Width of the Game Controls' row and the room it has ("measure controls").
    struct ControlsWidth
    {
        float content = 0.0f;
        float room = 0.0f;
    };

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
        std::map<std::string, PanelPlace> panels;
        std::vector<std::size_t> objectCounts;
        std::vector<TreeScroll> hierarchyScrolls;
        std::vector<ControlsWidth> controls;

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
                } else if (action == "select last") {
                    // Like the Viewport does: from outside the Hierarchy.
                    selected = hierarchy().roots().back();
                    selection.set(selected);
                } else if (action == "focus hierarchy search") {
                    ImGui::ActivateItemByID(ImGui::FindWindowByName("Hierarchy")->GetID("##HierarchySearch"));
                } else if (action.starts_with("type ")) {
                    ImGui::GetIO().AddInputCharactersUTF8(action.substr(std::string_view("type ").size()).c_str());
                } else if (action == "leave text field") {
                    ImGui::ClearActiveID();
                } else if (action.starts_with("focus ")) {
                    // Selects the panel's tab, so it is drawn.
                    ImGui::SetWindowFocus(action.substr(std::string_view("focus ").size()).c_str());
                } else if (action.starts_with("remove file ")) {
                    std::filesystem::remove(action.substr(std::string_view("remove file ").size()));
                } else if (action.starts_with("create file ")) {
                    const std::filesystem::path file = action.substr(std::string_view("create file ").size());
                    if (file.has_parent_path())
                        std::filesystem::create_directories(file.parent_path());
                    std::ofstream(file) << "test";
                } else if (action == "press F9" || action == "release F9") {
                    ImGui::GetIO().AddKeyEvent(ImGuiKey_F9, action == "press F9");
                } else if (action == "count objects") {
                    objectCounts.push_back(hierarchy().allObjects().size());
                } else if (action == "measure hierarchy scroll") {
                    for (const ImGuiWindow *window : ImGui::GetCurrentContext()->Windows)
                        if (std::string_view(window->Name).starts_with("Hierarchy/##HierarchyTree"))
                            hierarchyScrolls.push_back({window->Scroll.y, window->ScrollMax.y});
                } else if (action == "measure controls") {
                    if (const ImGuiWindow *window = ImGui::FindWindowByName("Game Controls"))
                        controls.push_back({window->ContentSize.x, window->ContentRegionRect.GetWidth()});
                } else if (action == "measure panels") {
                    for (const char *name : {"Hierarchy", "Entity Palette"})
                        if (const ImGuiWindow *window = ImGui::FindWindowByName(name))
                            panels[name] = {window->DockIsActive, window->Pos};
                } else if (action == "quit") {
                    _gui.stop();
                } else {
                    _commands->execute(action);
                }
            }
        }
    };

    struct Run
    {
        int frames = 0;
        HierarchyModel::ObjectId selected = HierarchyModel::InvalidId;
        std::map<std::string, PanelPlace> panels;
        std::vector<std::size_t> objectCounts;
        std::vector<TreeScroll> hierarchyScrolls;
        std::vector<ControlsWidth> controls;
        // The imgui.ini file the editor saved when it closed.
        std::string savedIni;
    };

    struct RunOptions
    {
        // Written as the imgui.ini file before the editor opens, if set.
        std::string savedIni;
        int width = 1280;
        int height = 720;
    };

    // Opens the editor on world and runs script until it quits.
    Run runEditor(ECS::Registry &world, ECS::SystemManager &systems, Script script, const RunOptions &options = {})
    {
        FreshWorkingDirectory directory;
        if (!options.savedIni.empty())
            std::ofstream("imgui.ini") << options.savedIni;
        GuiContext gui("tests_editor", options.width, options.height);
        auto backend = std::make_unique<ScriptedBackend>(gui, world, systems, std::move(script));
        ScriptedBackend &scripted = *backend;
        Run run;
        {
            Editor editor(gui, std::move(backend));
            editor.run();
            run.frames = scripted.frames;
            run.selected = scripted.selected;
            run.panels = scripted.panels;
            run.objectCounts = scripted.objectCounts;
            run.hierarchyScrolls = scripted.hierarchyScrolls;
            run.controls = scripted.controls;
        }
        std::ifstream saved("imgui.ini");
        run.savedIni.assign(std::istreambuf_iterator<char>(saved), std::istreambuf_iterator<char>());
        return run;
    }

    // The ini file without a window's section: as saved before it existed.
    std::string withoutWindow(const std::string &ini, const std::string &name)
    {
        const std::size_t start = ini.find("[Window][" + name + "]\n");
        if (start == std::string::npos)
            return ini;
        const std::size_t end = ini.find("\n\n", start);
        return ini.substr(0, start) + (end == std::string::npos ? std::string() : ini.substr(end + 2));
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

    "a panel missing from a saved layout is docked in its area"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;
        const Script script{{10, {"measure panels"}}, {12, {"quit"}}};
        const Run first = runEditor(world, systems, script);
        const std::string beforePalette = withoutWindow(first.savedIni, "Entity Palette");
        expect(beforePalette.size() < first.savedIni.size()) << "the first run saved the palette";

        Run second = runEditor(world, systems, script, {.savedIni = beforePalette});

        expect(second.panels.contains("Entity Palette") && second.panels.contains("Hierarchy"));
        const PanelPlace palette = second.panels["Entity Palette"];
        const PanelPlace hierarchy = second.panels["Hierarchy"];
        expect(palette.docked) << "not floating";
        expect(std::abs(palette.pos.x - hierarchy.pos.x) < 1.0f && palette.pos.y > hierarchy.pos.y) << "below the Hierarchy";
    };

    "objects cannot be created during Play, even with the shortcut"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;
        world.spawnEntity("A");

        Logger::instance().clear();
        const Run run = runEditor(world, systems,
            {
                {3, {"bind edit.create F9", "play"}},
                {5, {"press F9"}},
                {6, {"release F9"}},
                {9, {"count objects", "stop"}},
                {11, {"press F9"}},
                {12, {"release F9"}},
                {15, {"count objects"}},
                {17, {"quit"}},
            });

        expect(loggedErrors() == 0_ul) << "the shortcut was bound";
        expect(run.objectCounts.size() == 2_ul);
        expect(run.objectCounts.size() == 2 && run.objectCounts[0] == 1) << "nothing created during Play";
        expect(run.objectCounts.size() == 2 && run.objectCounts[1] == 2) << "the shortcut creates once stopped";
    };

    "an object selected outside the Hierarchy is scrolled to there"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;
        for (int i = 0; i < 300; i++)
            world.spawnEntity("Entity " + std::to_string(i));

        const Run run = runEditor(world, systems, {{3, {"select last"}}, {10, {"measure hierarchy scroll"}}, {12, {"quit"}}});

        expect(run.hierarchyScrolls.size() == 1 && run.hierarchyScrolls[0].position > 0.0f)
            << "the last of 300 rows is far below the first ones";
    };

    "an object hidden by the Hierarchy's search is shown when selected elsewhere"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;
        for (int i = 0; i < 300; i++)
            world.spawnEntity("Entity " + std::to_string(i));

        // "Entity 1" lists Entity 1, 10 to 19 and 100 to 199, not Entity 299.
        const Run run = runEditor(world, systems,
            {
                {3, {"focus hierarchy search"}},
                {5, {"type Entity 1"}},
                {7, {"leave text field"}},
                {9, {"measure hierarchy scroll", "select last"}},
                {14, {"measure hierarchy scroll"}},
                {16, {"quit"}},
            });

        expect(run.hierarchyScrolls.size() == 2_ul);
        if (run.hierarchyScrolls.size() != 2)
            return;
        const TreeScroll filtered = run.hierarchyScrolls[0];
        const TreeScroll shown = run.hierarchyScrolls[1];
        expect(filtered.max > 0.0f && shown.max > filtered.max) << "the search hid rows, then was cleared";
        expect(shown.position > 0.0f) << "scrolled to the selected object";
    };

    "the game controls do not move when the game plays or pauses"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;

        const Run run = runEditor(world, systems,
            {
                {8, {"measure controls", "play"}},
                {12, {"measure controls", "pause"}},
                {16, {"measure controls"}},
                {18, {"quit"}},
            });

        expect(run.controls.size() == 3_ul);
        for (const ControlsWidth &measure : run.controls)
            expect(std::abs(measure.content - run.controls.front().content) < 0.5f) << "same row in every state";
    };

    "the game controls fit in the smallest window"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;

        // The window's minimum size (see GuiContext).
        const Run run = runEditor(
            world, systems, {{8, {"play"}}, {12, {"measure controls"}}, {14, {"quit"}}}, {.width = 640, .height = 400});

        expect(run.controls.size() == 1_ul);
        expect(run.controls.size() == 1 && run.controls[0].content <= run.controls[0].room + 0.5f)
            << "nothing cut off on the right";
    };

    "levels are saved, created, opened and renamed from the console"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;
        world.spawnEntity("A");

        Logger::instance().clear();
        const Run run = runEditor(world, systems,
            {
                {3, {"level save first", "level new second", "count objects"}},
                {5, {"level open first", "count objects", "level rename first renamed", "level list"}},
                {7, {"quit"}},
            });

        expect(loggedErrors() == 0_ul);
        expect(run.objectCounts == std::vector<std::size_t>{0, 1}) << "the new level is empty, the first one has A";
        expect(logged(LogLevel::Info, "Levels: renamed, second"));
    };

    "levels do not change during Play, nor over unsaved changes"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;
        world.spawnEntity("A");

        Logger::instance().clear();
        const Run run = runEditor(world, systems,
            {
                {3, {"level new second"}},
                {4, {"level save first", "play"}},
                {6, {"level new third"}},
                {8, {"stop", "count objects", "level list"}},
                {10, {"quit"}},
            });

        expect(logged(LogLevel::Error, "unsaved changes")) << "A is in no level yet";
        expect(logged(LogLevel::Error, "stop the game first"));
        expect(loggedErrors() == 2_ul);
        expect(run.objectCounts == std::vector<std::size_t>{1});
        expect(logged(LogLevel::Info, "Levels: first")) << "only the level saved before Play";
    };

    "the running editor reads the assets folder"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;

        Logger::instance().clear();
        runEditor(world, systems,
            {
                {3, {"create file assets/sprites/hero.png", "create file assets/jump.wav", "create file outside.png", "focus Assets"}},
                {6, {"assets"}},
                {8, {"quit"}},
            });

        expect(logged(LogLevel::Info, "2 assets in")) << "the files of assets/ and its subfolders only";
        expect(loggedErrors() == 0_ul) << "the Asset Browser drew them";
    };

    "the Asset Browser follows a scan made by the assets command"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;

        // The panel shows two files; the command reads the folder again
        // with one, between two of the panel's own scans.
        Logger::instance().clear();
        runEditor(world, systems,
            {
                {3, {"create file assets/a.png", "create file assets/b.png", "focus Assets"}},
                {6, {"remove file assets/b.png", "assets"}},
                {10, {"quit"}},
            });

        expect(logged(LogLevel::Info, "1 assets in"));
        expect(loggedErrors() == 0_ul);
    };
}
