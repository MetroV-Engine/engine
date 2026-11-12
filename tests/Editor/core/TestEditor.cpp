#include <boost/ut.hpp>

#include "Support/Headless.hpp"
#include "core/Editor/Editor.hpp"
#include "ecs/TestComponents.hpp"
#include "scene/DemoBackend.hpp"

#include "ECS/ECS.hpp"

#include <SDL3/SDL.h>

#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>

using namespace boost::ut;
using namespace EditorTest;

namespace {

// Runs the editor until the script ends, one step per frame from the
// Viewport's renderer (rendered small, see renderSmall), then stops it. A watchdog quits after a minute if the
// script never ends (e.g. the Viewport was hidden).
void runEditor(Gui &gui, Editor &editor, Script &script, bool stopAtEnd = true)
{
    editor.setViewportRenderer([&](ImDrawList *, ImVec2, ImVec2) {
        renderSmall();
        if (!script.step() && stopAtEnd)
            gui.stop();
    });
    const SDL_TimerID watchdog = SDL_AddTimer(
        60000,
        [](void *, SDL_TimerID, Uint32) -> Uint32 {
            SDL_Event quit{};
            quit.type = SDL_EVENT_QUIT;
            SDL_PushEvent(&quit);
            return 0;
        },
        nullptr);
    editor.run();
    SDL_RemoveTimer(watchdog);
}

std::string readFile(const std::string &path)
{
    std::ifstream file(path);
    return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

// A fresh start: no saved layout, workspace or shortcuts.
void forgetSavedState()
{
    for (const char *file : {"imgui.ini", "imgui-demo.ini", "shortcuts.ini"})
        std::filesystem::remove(file);
}

// The menu bar's menus, and an item of the open menu.
ImGuiID menu(const char *name)
{
    return itemId(itemId(window("##MainMenuBar"), {"##MenuBar"}), {name});
}

ImGuiID menuItem(int depth, const std::string &label)
{
    return itemId(menuWindow(depth), {label});
}

void pushWindowEvent(SDL_EventType type)
{
    SDL_Event event{};
    event.type = type;
    SDL_PushEvent(&event);
}

// Adds gold to every EdStats at each update.
class GoldSystem : public ECS::ISystem
{
public:
    void update(ECS::Registry &world, double) override
    {
        for (auto [entity, stats] : world.view<EdStats>())
            stats.gold += 1;
    }
    const std::string name() const override { return "GoldSystem"; }
};

} // namespace

void run_editor_tests()
{
    "every console command of the editor, typed in the console"_test = [] {
        forgetSavedState();
        ECS::Registry world;
        ECS::SystemManager systems;
        systems.addSystem<GoldSystem>(world);
        const ECS::Entity hero = world.spawnEntity("Hero");
        world.addComponent(hero, EdStats{});
        Gui gui;
        Editor editor(gui, world, systems);
        LogWatch log;

        Script script;
        script.wait(6).keys(ImGuiKey_GraveAccent, 4);
        for (const char *line : {"help", R"(echo "hello world")", "fps", "theme Light", "theme Nope", "theme",
                 "vsync on", "vsync off", "vsync", "vsync 1", "vsync false", "vsync maybe", "fullscreen on",
                 "fullscreen off", "events on", "events all", "events off", "events nope", "log warning a test warning",
                 "log nope x", "log x", "shortcuts", "shortcuts bad", "bind view.palette Ctrl+K", "bind view.palette",
                 "bind nope Ctrl+K", "bind view.palette Nope", "bind view.palette Escape", "bind view.palette F5",
                 "unbind view.palette Ctrl+K", "unbind view.palette Ctrl+J", "unbind view.palette", "unbind",
                 "unbind nope", "shortcuts reset", "workspace save", "workspace bad", "objects", "components",
                 "play", "pause", "step 3", "step 0", "timescale 2", "timescale", "stop", "log error console error"})
            script.enter(line);
        // Errors of other categories get a notification.
        script.then([] { Logger::instance().error("Engine", "engine failure"); }).wait(3);
        script.then([] {
            Logger::instance().error("Physics", "first");
            Logger::instance().error("Physics", "second");
        });
        // "clear" empties the log: what it held is kept to check it.
        std::string logged;
        script.wait(3).then([&] { logged = log.text(); }).enter("clear");
        // Commands that ask first. Escape twice: the command line takes the first one.
        script.enter("workspace reset").keys(ImGuiKey_Enter, 4).keys(ImGuiKey_GraveAccent, 4);
        script.enter("quit").keys(ImGuiKey_Escape, 4).keys(ImGuiKey_Escape, 4).keys(ImGuiKey_GraveAccent, 4);
        bool reachedEnd = false;
        script.then([&] { reachedEnd = true; }).enter("quit").keys(ImGuiKey_Enter, 4);
        runEditor(gui, editor, script, false);

        expect(reachedEnd) << log.text();
        expect(log.contains("Workspace reset"));
        expect(log.contains("> quit"));
        for (const char *expected : {"echo <text>  -  Print a message", "hello world", " FPS, ", "available themes: Dark, Light",
                 "VSync on", "VSync off", "expected on or off, got \"maybe\"", "Event logging: on", "Event logging: all",
                 "expected off, on or all", "a test warning", "unknown level \"nope\"", "missing level or message",
                 "view.palette  Ctrl+Shift+P", "unknown option \"bad\"", "Ctrl+K -> view.palette",
                 "expected an action id and a shortcut", "unknown action \"nope\"", "invalid shortcut \"Nope\"",
                 "Escape cancels and closes popups", "F5 is used by game.play", "Shortcut removed from view.palette",
                 "has no shortcut \"Ctrl+J\"", "expected an action id and optionally a shortcut",
                 "Shortcuts reset to the defaults", "Workspace saved to imgui.ini", "expected save or reset",
                 "1 objects in the scene", "component types", "Playing", "Paused", "frames must be between 1 and 10000",
                 "Time scale: x2", "expected a factor", "Stopped: the scene is back to its state before Play",
                 "Error in Engine: engine failure", "2 new errors: second"}) {
            expect(logged.find(expected) != std::string::npos) << expected;
        }
        // "step 3" played three frames; "stop" put the world back.
        expect(world.getComponent<EdStats>(hero).gold == 0);
        expect(readFile("imgui.ini").find("[Workspace][Editor]") != std::string::npos);
        expect(std::filesystem::exists("shortcuts.ini"));
    };

    "shortcuts, menus, toolbar, palette and play mode"_test = [] {
        forgetSavedState();
        ECS::Registry world;
        ECS::SystemManager systems;
        systems.addSystem<GoldSystem>(world);
        const ECS::Entity hero = world.spawnEntity("Hero");
        world.addComponent(hero, EdStats{});
        Gui gui;
        Editor editor(gui, world, systems);
        LogWatch log;
        bool fullscreenSeen = false;
        std::string themeSeen;

        Script script;
        script.wait(6);
        // Create an object (Ctrl+N): it is selected and renamed.
        script.keys(ImGuiMod_Ctrl | ImGuiKey_N, 6).enter("Player");
        // Play, pause, resume, step, stop.
        script.keys(ImGuiKey_F5, 10).keys(ImGuiKey_F6).keys(ImGuiKey_F6, 6).keys(ImGuiKey_F6).keys(ImGuiKey_F10)
            .keys(ImGuiMod_Shift | ImGuiKey_F5);
        // View toggles.
        script.keys(ImGuiKey_F3).keys(ImGuiKey_F3).keys(ImGuiKey_F11, 4);
        script.then([&] { fullscreenSeen = gui.isFullscreen(); }).keys(ImGuiKey_F11, 4);
        script.keys(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_T);
        script.then([&] { themeSeen = gui.themes().current().name; }).keys(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_T);
        script.keys(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_R, 6);
        script.keys(ImGuiKey_F1, 4).keys(ImGuiKey_F1);
        for (const ImGuiKey key : {ImGuiKey_1, ImGuiKey_2, ImGuiKey_3, ImGuiKey_4, ImGuiKey_5})
            script.keys(ImGuiMod_Ctrl | key);
        // The toolbar hides the Hierarchy; Ctrl+1 shows it again.
        script.then([] { activate(itemId(window("##Toolbar"), {"Hierarchy##view.panel.Hierarchy"})); }).wait(3);
        script.keys(ImGuiMod_Ctrl | ImGuiKey_1, 4);
        script.then([] { activate(itemId(window("##Toolbar"), {"Console##view.panel.Console"})); }).wait(3);
        script.then([] { activate(itemId(window("##Toolbar"), {"Console##view.panel.Console"})); }).wait(3);
        // The command palette runs a command found by a few letters.
        script.keys(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_P, 4).enter("status bar");
        script.keys(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_P, 4).enter("status bar");
        // Every menu and submenu; Edit > Rename and Delete on the selection.
        for (const char *name : {"File", "Window", "Game", "Help"})
            script.then([name] { activate(menu(name)); }).wait(3).keys(ImGuiKey_Escape);
        script.then([] { activate(menu("View")); }).wait(3);
        script.then([] { activate(menuItem(0, "Panels")); }).wait(3);
        script.then([] { activate(menuItem(1, "Settings##view.panel.Settings")); }).wait(3);
        script.then([] { activate(menu("View")); }).wait(3);
        script.then([] { activate(menuItem(0, "Theme")); }).wait(3);
        script.then([] { activate(menuItem(1, "Light##view.theme.Light")); }).wait(3);
        script.then([] { activate(menu("Help")); }).wait(3);
        script.then([] { activate(menuItem(0, "About##view.panel.About")); }).wait(3);
        script.then([] { activate(menu("Edit")); }).wait(3);
        script.then([] { activate(menuItem(0, "Rename##edit.rename")); }).wait(6).keys(ImGuiKey_Escape, 4);
        script.then([] { activate(menu("Edit")); }).wait(3);
        script.then([] { activate(menuItem(0, "Delete##edit.delete")); }).wait(4).keys(ImGuiKey_Enter, 4);
        script.then([] { activate(menu("View")); }).wait(3);
        script.then([] { activate(menuItem(0, "Reset Workspace##view.workspace.reset")); }).wait(4).keys(ImGuiKey_Escape, 4);
        // The status bar's counts open the console.
        script.then([] {
            ImGuiWindow *bar = window("##StatusBar");
            const float width = bar->StateStorage.GetFloat(itemId(bar, {"##MenuBar", "##StatusBarItemsWidth"}), 0.0f);
            click(ImVec2(bar->ContentRegionRect.Max.x - width + 8.0f, bar->Pos.y + bar->Size.y * 0.5f));
        });
        script.wait(5);
        // Closing the window asks first; asking twice shows one dialog.
        bool reachedEnd = false;
        script.then([&] {
            reachedEnd = true;
            pushWindowEvent(SDL_EVENT_WINDOW_CLOSE_REQUESTED);
        });
        script.wait(3);
        script.keys(ImGuiMod_Ctrl | ImGuiKey_Q, 3).keys(ImGuiKey_Escape, 4);
        script.keys(ImGuiMod_Ctrl | ImGuiKey_Q, 3).keys(ImGuiKey_Enter, 4);
        runEditor(gui, editor, script, false);

        expect(reachedEnd) << log.text();
        expect(fullscreenSeen);
        expect(themeSeen == "Light");
        expect(log.contains("Playing"));
        expect(log.contains("Paused"));
        expect(log.contains("Stopped"));
        expect(log.contains("Layout reset"));
        expect(log.contains("Deleted"));
        // Play changed the gold, Stop put it back.
        expect(world.getComponent<EdStats>(hero).gold == 0);
        bool playerCreated = false;
        for (auto [entity, name] : world.view<ECS::Name>())
            playerCreated |= name.value == "Player";
        expect(!playerCreated); // Created, then deleted from the Edit menu
    };

    "the demo editor: demo panels, the demo level and its game"_test = [] {
        forgetSavedState();
        LogWatch log;
        bool reachedEnd = false;
        {
            Gui gui;
            Editor editor(gui, Editor::Options{.demo = true});
            Script script;
            script.wait(6);
            script.then([] { activate(menu("Help")); }).wait(3);
            script.then([] { activate(menuItem(0, "Dear ImGui Demo##view.panel.Dear ImGui Demo")); }).wait(4);
            // The Buttons panel is a tab behind the Console: brought to the front.
            script.then([] { ImGui::SetWindowFocus("Buttons"); }).wait(3);
            for (const char *button : {"Say hello", "Warning", "Error"})
                script.then([button] { activate(itemId(window("Buttons"), {button})); }).wait(3);
            script.then([] { activate(itemId(window("Buttons"), {"Alert"})); }).wait(3).keys(ImGuiKey_Enter, 4);
            script.keys(ImGuiKey_F5, 20).keys(ImGuiMod_Shift | ImGuiKey_F5);
            script.keys(ImGuiKey_GraveAccent, 4).enter("objects");
            script.then([] { ImGui::SetWindowFocus("Buttons"); }).wait(3);
            script.then([&] {
                reachedEnd = true;
                activate(itemId(window("Buttons"), {"Quit"}));
            });
            script.wait(3).keys(ImGuiKey_Enter, 4);
            runEditor(gui, editor, script, false);
        } // The layout is saved when the editor closes.
        expect(reachedEnd) << log.text();
        expect(log.contains("Editor started in demo mode"));
        expect(log.contains("Editing the built-in demo level"));
        expect(log.contains("Hello!: Everything works."));
        expect(log.contains("objects in the scene"));
        expect(std::filesystem::exists("imgui-demo.ini"));
    };

    "the workspace and shortcuts saved by a previous session are restored"_test = [] {
        forgetSavedState();
        std::ofstream("imgui.ini") << "[Workspace][Editor]\n"
                                      "window.width=900\nwindow.height=600\nwindow.x=10\nwindow.y=10\n"
                                      "window.maximized=0\nwindow.fullscreen=0\n"
                                      "editor.theme=Light\neditor.vsync=0\neditor.statusBar=0\neditor.performanceOverlay=0\n"
                                      "panel.Console=0\npanel.Settings=1\n"
                                      "console.levels=11011\nconsole.hiddenCategories=SDL,,Game\nconsole.autoScroll=0\n\n";
        std::ofstream("shortcuts.ini") << "view.palette=Ctrl+K\nnot.an.action=F5\n";
        LogWatch log;
        {
            Gui gui;
            Editor editor(gui);
            expect(gui.themes().current().name == "Light");
            expect(gui.getWindowState().width == 900_i);
            Script script;
            script.wait(3).keys(ImGuiMod_Ctrl | ImGuiKey_K, 4).keys(ImGuiKey_Escape, 4);
            runEditor(gui, editor, script);
        }
        expect(log.contains("Workspace restored from imgui.ini"));
        expect(log.contains("Loaded 1 custom shortcut(s) from shortcuts.ini"));
        expect(log.contains(LogLevel::Warning, "unknown action \"not.an.action\""));
        expect(log.contains("Editing an empty built-in demo scene"));
        const std::string saved = readFile("imgui.ini");
        expect(saved.find("console.hiddenCategories=Game,SDL") != std::string::npos) << saved;
        expect(saved.find("console.levels=11011") != std::string::npos);
        expect(saved.find("panel.Console=0") != std::string::npos);
        expect(saved.find("editor.theme=Light") != std::string::npos);
    };

    "an editor needs a scene; shortcuts it cannot save are reported"_test = [] {
        forgetSavedState();
        Gui gui;
        expect(throws<std::invalid_argument>([&] { Editor editor(gui, std::unique_ptr<SceneBackend>()); }));

        // shortcuts.ini is a directory: the change cannot be written.
        std::filesystem::create_directory("shortcuts.ini");
        LogWatch log;
        {
            Editor editor(gui, std::make_unique<DemoBackend>(false));
            Script script;
            script.wait(3).keys(ImGuiKey_GraveAccent, 4).enter("bind view.palette Ctrl+J");
            runEditor(gui, editor, script);
        }
        std::filesystem::remove("shortcuts.ini");
        expect(log.contains(LogLevel::Error, "Cannot write shortcuts.ini"));
    };
}
