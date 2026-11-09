/*
    Created on: 30/03/2026
    Filename: Editor.cpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "Editor.hpp"
#include "ecs/EcsBackend/EcsBackend.hpp"
#include "panels/ButtonsPanel/ButtonsPanel.hpp"
#include "panels/ConsolePanel/ConsolePanel.hpp"
#include "panels/EntityPalettePanel/EntityPalettePanel.hpp"
#include "panels/HierarchyPanel/HierarchyPanel.hpp"
#include "panels/ImGuiDemoPanel.hpp"
#include "panels/InputsPanel/InputsPanel.hpp"
#include "panels/InspectorPanel/InspectorPanel.hpp"
#include "panels/PlayControlsPanel/PlayControlsPanel.hpp"
#include "panels/SettingsPanel/SettingsPanel.hpp"
#include "panels/ShortcutsPanel/ShortcutsPanel.hpp"
#include "panels/ViewportPanel/ViewportPanel.hpp"
#include "core/Logger/Logger.hpp"
#include "scene/DemoBackend.hpp"

#include <imgui.h>

#include <algorithm>
#include <cstdio>
#include <iterator>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

Editor::Editor(GuiContext &gui, ECS::Registry &world, ECS::SystemManager &systems, Options options)
    : Editor(gui, std::make_unique<EcsBackend>(world, systems), options)
{
}

Editor::Editor(GuiContext &gui, Options options) : Editor(gui, std::make_unique<DemoBackend>(options.demo), options)
{
}

Editor::Editor(GuiContext &gui, std::unique_ptr<SceneBackend> scene, Options options)
    : _gui(gui),
      _options(options),
      _performanceOverlay(gui.timer()),
      _scene(scene ? std::move(scene) : throw std::invalid_argument("Editor: no scene to edit")),
      _play(_scene->simulation()),
      _entityTemplates(_scene->entityTemplates()),
      _entityDropTarget(_entityTemplates, _scene->hierarchy(), _scene->properties(), _notifications, _play,
          [this](HierarchyModel::ObjectId id) { _hierarchy->showObject(id); }),
      _palette(_actions),
      _menuBar(_actions),
      _toolbar(_actions)
{
    // Tab / arrows / Space move between and activate widgets; Ctrl+Tab
    // switches panels.
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    // Each mode keeps its own panel layout, so the demo does not disturb the
    // normal one. Must be set before the first frame, when ImGui loads it.
    ImGui::GetIO().IniFilename = options.demo ? "imgui-demo.ini" : "imgui.ini";

    registerPanels();
    registerActions();
    registerPlayControls();
    buildMenuBar();
    buildToolbar();
    buildStatusBar();
    registerCommands();

    // Restored before the feedback is connected, so restoring the theme does
    // not show a "Theme changed" notification.
    _workspace = std::make_unique<Workspace>([this](Workspace::Values &values) { collectWorkspace(values); },
        [this](const Workspace::Values &values) { applyWorkspace(values); });
    collectWorkspace(_defaultWorkspace);
    _workspace->load();

    connectFeedback();
    loadShortcuts();
    _gui.setCloseRequestHandler([this] { requestQuit(); });
    logStartupInfo();
}

Editor::~Editor()
{
    // An ECS world outlives the editor: quitting during play must not leave
    // it in its play state.
    _play.stop();
    // The GuiContext may outlive the editor: its callbacks capture this.
    _gui.setCloseRequestHandler({});
    _gui.themes().setOnChange({});
}

void Editor::setViewportRenderer(SceneBackend::Renderer renderer)
{
    _viewport->setRenderer(std::move(renderer));
}

void Editor::addEntityTemplate(EntityTemplate entityTemplate)
{
    _entityTemplates.push_back(std::move(entityTemplate));
}

void Editor::registerPanels()
{
    using Area = EditorLayout::Area;

    _hierarchy = &_panels.registerPanel<HierarchyPanel>(_scene->hierarchy(), _selection, _notifications, _dialogs, _entityDropTarget);
    _layout.setDefaultArea("Hierarchy", Area::Left);

    _panels.registerPanel<EntityPalettePanel>(_entityTemplates, _play);
    // In its own area: as a tab of the Hierarchy, it would hide the drop target.
    _layout.setDefaultArea("Entity Palette", Area::LeftBottom);

    _viewport = &_panels.registerPanel<ViewportPanel>(
        _performanceOverlay, _play, _entityDropTarget, _scene->hierarchy(), _scene->properties(), _selection);
    _viewport->setRenderer(_scene->renderer(_selection));
    _layout.setDefaultArea("Viewport", Area::Center);

    _panels.registerPanel<InspectorPanel>(_scene->hierarchy(), _scene->properties(), _selection, _notifications).setBanner([this] {
        return _play.isStopped() ? std::string() : std::string("Play mode: changes are lost when the game stops.");
    });
    _layout.setDefaultArea("Inspector", Area::Right);
    _panels.registerPanel<SettingsPanel>(_gui, _performanceOverlay);
    _layout.setDefaultArea("Settings", Area::Right);

    _panels.registerPanel<PlayControlsPanel>(_actions, _play).setOnResetPosition([this] {
        _panels.setVisible("Game Controls", true);
        _layout.resetLayout();
    });
    _layout.setDefaultArea("Game Controls", Area::Controls);
    _console = &_panels.registerPanel<ConsolePanel>(_commands);
    _layout.setDefaultArea("Console", Area::Bottom);
    // Widget demos, next to the console.
    _layoutPanels = {"Hierarchy", "Entity Palette", "Viewport", "Inspector", "Game Controls", "Console", "Settings"};

    if (_options.demo) {
        _panels.registerPanel<ButtonsPanel>(_notifications, _dialogs, [this] { requestQuit(); });
        _layout.setDefaultArea("Buttons", Area::Bottom);
        _panels.registerPanel<InputsPanel>();
        _layout.setDefaultArea("Inputs", Area::Bottom);
        _layoutPanels.insert(_layoutPanels.end(), {"Buttons", "Inputs"});
        // Floating, hidden until opened from the Help menu.
        _panels.registerPanel<ImGuiDemoPanel>().setVisible(false);
    }

    // Floating, hidden until opened from the Help menu.
    _shortcutsPanel = &_panels.registerPanel<ShortcutsPanel>(_actions, _dialogs, _notifications);
    _shortcutsPanel->setVisible(false);
    _panels.registerPanel("About", [] {
        ImGui::TextUnformatted("MetroV Engine - Editor");
        ImGui::TextDisabled("Built with SDL3 and Dear ImGui %s", IMGUI_VERSION);
    }, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDocking).setVisible(false);
}

void Editor::registerActions()
{
    _actions.add({.id = "file.quit", .label = "Quit", .callback = [this] { requestQuit(); },
        .shortcut = ImGuiMod_Ctrl | ImGuiKey_Q});

    for (const auto &panel : _panels.getPanels()) {
        Panel *p = panel.get();
        _actions.add({.id = "view.panel." + p->getName(), .label = p->getName(),
            .callback = [p] { p->setVisible(!p->isVisible()); }, .tooltip = "Show/hide the " + p->getName() + " panel",
            .isChecked = [p] { return p->isVisible(); }});
    }
    _actions.add({.id = "edit.create", .label = "Create Object",
        .callback = [this] { _hierarchy->createObject(HierarchyModel::InvalidId); },
        .shortcut = ImGuiMod_Ctrl | ImGuiKey_N, .tooltip = "Create an object at the root of the scene"});
    // F2 and Delete are handled by the Hierarchy when it has focus (so typing
    // elsewhere never deletes an object); the menu shows them as hints.
    _actions.add({.id = "edit.rename", .label = "Rename", .callback = [this] { _hierarchy->startRename(_selection.get()); },
        .isEnabled = [this] { return _selection.has(); }, .shortcutHint = "F2"});
    _actions.add({.id = "edit.delete", .label = "Delete", .callback = [this] { _hierarchy->requestDelete(_selection.get()); },
        .isEnabled = [this] { return _selection.has(); }, .shortcutHint = "Del"});

    _actions.add({.id = "view.palette", .label = "Command Palette", .callback = [this] { _palette.toggle(); },
        .shortcut = ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_P, .tooltip = "Search and run any command"});
    _actions.add({.id = "help.shortcuts", .label = "Keyboard Shortcuts",
        .callback = [this] { _panels.toggle("Keyboard Shortcuts"); }, .shortcut = ImGuiKey_F1,
        .isChecked = [this] { return _panels.getPanel("Keyboard Shortcuts")->isVisible(); }});

    // Ctrl+1..5: jump to a panel.
    const std::pair<const char *, ImGuiKey> focusKeys[] = {{"Hierarchy", ImGuiKey_1}, {"Viewport", ImGuiKey_2},
        {"Inspector", ImGuiKey_3}, {"Console", ImGuiKey_4}, {"Settings", ImGuiKey_5}};
    for (const auto &[name, key] : focusKeys) {
        const std::string panel = name;
        _actions.add({.id = "window.focus." + panel, .label = "Focus " + panel, .callback = [this, panel] { focusPanel(panel); },
            .shortcut = ImGuiMod_Ctrl | key});
    }

    _actions.add({.id = "view.console", .label = "Developer Console",
        .callback = [this] {
            _panels.setVisible("Console", true);
            ImGui::SetWindowFocus("Console");
            _console->focusCommandLine();
        },
        .shortcut = ImGuiKey_GraveAccent, .tooltip = "Open the console and type a command"});

    _actions.add({.id = "view.theme.next", .label = "Next Theme",
        .callback = [this] {
            const std::vector<std::string> names = _gui.themes().getThemeNames();
            const auto it = std::ranges::find(names, _gui.themes().current().name);
            const std::size_t next = it == names.end() ? 0 : (static_cast<std::size_t>(it - names.begin()) + 1) % names.size();
            _gui.themes().setTheme(names[next]);
        },
        .shortcut = ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_T, .tooltip = "Switch to the next theme"});

    _actions.add({.id = "view.layout.reset", .label = "Reset Layout",
        .callback = [this] {
            for (const std::string &name : _layoutPanels)
                _panels.setVisible(name, true);
            _layout.resetLayout();
            _notifications.info("Layout reset");
        },
        .shortcut = ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_R, .tooltip = "Restore the default panel arrangement"});
    _actions.add({.id = "view.workspace.reset", .label = "Reset Workspace", .callback = [this] { resetWorkspace(); },
        .tooltip = "Restore the default window, panels, theme and layout"});
    _actions.add({.id = "view.statusbar", .label = "Status Bar",
        .callback = [this] { _statusBar.setVisible(!_statusBar.isVisible()); },
        .isChecked = [this] { return _statusBar.isVisible(); }});
    _actions.add({.id = "view.fullscreen", .label = "Fullscreen",
        .callback = [this] { _gui.setFullscreen(!_gui.isFullscreen()); }, .shortcut = ImGuiKey_F11,
        .tooltip = "Toggle fullscreen", .isChecked = [this] { return _gui.isFullscreen(); }});
    _actions.add({.id = "view.performance", .label = "Performance",
        .callback = [this] { _performanceOverlay.setVisible(!_performanceOverlay.isVisible()); },
        .shortcut = ImGuiKey_F3, .tooltip = "Show FPS and frame time",
        .isChecked = [this] { return _performanceOverlay.isVisible(); }});

    ThemeManager &themes = _gui.themes();
    for (const std::string &name : themes.getThemeNames())
        _actions.add({.id = "view.theme." + name, .label = name, .callback = [&themes, name] { themes.setTheme(name); },
            .isChecked = [&themes, name] { return themes.current().name == name; }});
}

void Editor::buildMenuBar()
{
    _menuBar.addAction("File", "file.quit");

    _menuBar.addAction("Edit", "edit.create");
    _menuBar.addAction("Edit", "edit.rename");
    _menuBar.addAction("Edit", "edit.delete");

    for (const std::string &name : _layoutPanels)
        _menuBar.addAction("View/Panels", "view.panel." + name);
    _menuBar.addAction("View", "view.palette");
    _menuBar.addAction("View", "view.console");
    _menuBar.addAction("View", "view.layout.reset");
    _menuBar.addAction("View", "view.workspace.reset");
    _menuBar.addSeparator("View");
    _menuBar.addAction("View", "view.fullscreen");
    _menuBar.addAction("View", "view.performance");
    _menuBar.addAction("View", "view.statusbar");
    for (const std::string &name : _gui.themes().getThemeNames())
        _menuBar.addAction("View/Theme", "view.theme." + name);
    _menuBar.addSeparator("View/Theme");
    _menuBar.addAction("View/Theme", "view.theme.next");
    for (const std::string &name : _layoutPanels)
        if (_actions.has("window.focus." + name))
            _menuBar.addAction("Window", "window.focus." + name);

    _menuBar.addAction("Game", "game.play");
    _menuBar.addAction("Game", "game.pause");
    _menuBar.addAction("Game", "game.stop");
    _menuBar.addSeparator("Game");
    _menuBar.addAction("Game", "game.step");

    if (_options.demo)
        _menuBar.addAction("Help", "view.panel.Dear ImGui Demo");
    _menuBar.addAction("Help", "help.shortcuts");
    _menuBar.addAction("Help", "view.panel.About");
}

void Editor::buildToolbar()
{
    _toolbar.addAction("view.panel.Hierarchy");
    _toolbar.addAction("view.panel.Inspector");
    _toolbar.addAction("view.panel.Console");
    _toolbar.addSeparator();
    _toolbar.addAction("view.fullscreen");
    _toolbar.addAction("view.performance");
}

void Editor::buildStatusBar()
{
    // Warning and error counts; a click opens the console.
    _statusBar.addItem([this] {
        const Theme &theme = _gui.themes().current();
        const Logger &logger = Logger::instance();
        const std::size_t warnings = logger.count(LogLevel::Warning);
        const std::size_t errors = logger.count(LogLevel::Error);
        ImGui::TextColored(warnings ? theme.colors.warning : theme.colors.textMuted, "%zu warning%s", warnings,
            warnings == 1 ? "" : "s");
        const bool clicked = ImGui::IsItemClicked();
        ImGui::SameLine();
        ImGui::TextColored(errors ? theme.colors.danger : theme.colors.textMuted, "%zu error%s", errors,
            errors == 1 ? "" : "s");
        if (clicked || ImGui::IsItemClicked())
            _actions.execute("view.console");
    });
    if (_options.demo)
        _statusBar.addItem([this] { ImGui::TextColored(_gui.themes().current().colors.accent, "DEMO"); });
    _statusBar.addItem([this] {
        const Theme &theme = _gui.themes().current();
        const PlayState state = _play.getState();
        const ImVec4 &color = state == PlayState::Playing ? theme.colors.success
            : state == PlayState::Paused                  ? theme.colors.warning
                                                          : theme.colors.textMuted;
        if (state == PlayState::Stopped)
            ImGui::TextColored(color, "Editing");
        else
            ImGui::TextColored(color, "%s %.1fs  x%.2g", PlayController::stateName(state), _play.getPlayTime(), _play.getTimeScale());
    });
    _statusBar.addItem([this] { ImGui::Text("%.0f FPS", _gui.timer().getFps()); });
    _statusBar.addItem([this] { ImGui::TextUnformatted(_gui.themes().current().name.c_str()); });
}

void Editor::connectFeedback()
{
    // Every notification is also logged (category "Editor") and shown in the status bar.
    _notifications.setOnNotify(
        [this](NotificationManager::Level level, const std::string &title, const std::string &message) {
            const LogLevel logLevel = level == NotificationManager::Level::Success ? LogLevel::Success
                : level == NotificationManager::Level::Warning                   ? LogLevel::Warning
                : level == NotificationManager::Level::Error                     ? LogLevel::Error
                                                                                 : LogLevel::Info;
            Logger::instance().log(logLevel, "Editor", message.empty() ? title : title + ": " + message);
            _statusBar.setMessage(title);
        });
    _gui.themes().setOnChange([this](const Theme &theme) { _notifications.info("Theme changed", theme.name); });
}

void Editor::registerCommands()
{
    const auto onOff = [](const std::vector<std::string> &args, std::size_t index, bool current) {
        if (args.size() <= index)
            return !current; // No argument: toggle
        if (args[index] == "on" || args[index] == "1" || args[index] == "true")
            return true;
        if (args[index] == "off" || args[index] == "0" || args[index] == "false")
            return false;
        throw std::invalid_argument("expected on or off, got \"" + args[index] + "\"");
    };
    Logger &logger = Logger::instance();

    _commands.add("clear", "clear", "Clear the console", [](const auto &) { Logger::instance().clear(); });
    _commands.add("quit", "quit", "Quit the editor (asks for confirmation)", [this](const auto &) { requestQuit(); });
    _commands.add("fps", "fps", "Print the frame rate and frame time", [this, &logger](const auto &) {
        char text[96];
        std::snprintf(text, sizeof(text), "%.0f FPS, %.2f ms per frame (max %.2f ms)", _gui.timer().getFps(),
            _gui.timer().getAverageFrameTimeMs(), _gui.timer().getMaxFrameTimeMs());
        logger.info("Console", text);
    });
    _commands.add("theme", "theme <name>", "Switch the editor theme", [this](const std::vector<std::string> &args) {
        std::string names;
        for (const std::string &name : _gui.themes().getThemeNames())
            names += (names.empty() ? "" : ", ") + name;
        if (args.empty() || !_gui.themes().setTheme(args[0]))
            throw std::invalid_argument("available themes: " + names);
    });
    _commands.add("vsync", "vsync [on|off]", "Enable or disable VSync", [this, onOff](const std::vector<std::string> &args) {
        const bool enabled = onOff(args, 0, _gui.isVSync());
        _gui.setVSync(enabled);
        Logger::instance().info("Console", std::string("VSync ") + (enabled ? "on" : "off"));
    });
    _commands.add("fullscreen", "fullscreen [on|off]", "Enter or leave fullscreen",
        [this, onOff](const std::vector<std::string> &args) { _gui.setFullscreen(onOff(args, 0, _gui.isFullscreen())); });
    _commands.add("events", "events <off|on|all>", "Log SDL events (on: without mouse motion)",
        [this](const std::vector<std::string> &args) {
            using Logging = EventManager::Logging;
            const std::string mode = args.empty() ? "" : args[0];
            if (mode == "off")
                _gui.events().setLogging(Logging::Off);
            else if (mode == "on")
                _gui.events().setLogging(Logging::NoMotion);
            else if (mode == "all")
                _gui.events().setLogging(Logging::All);
            else
                throw std::invalid_argument("expected off, on or all");
            Logger::instance().info("Console", "Event logging: " + mode + " (Debug level, category Events)");
        });
    _commands.add("log", "log <debug|info|success|warning|error> <message>", "Write a message to the log, e.g. to test filters",
        [](const std::vector<std::string> &args) {
            if (args.size() < 2)
                throw std::invalid_argument("missing level or message");
            static const std::pair<const char *, LogLevel> levels[] = {{"debug", LogLevel::Debug},
                {"info", LogLevel::Info}, {"success", LogLevel::Success}, {"warning", LogLevel::Warning},
                {"error", LogLevel::Error}};
            const auto it = std::ranges::find(levels, args[0], [](const auto &level) { return std::string(level.first); });
            if (it == std::end(levels))
                throw std::invalid_argument("unknown level \"" + args[0] + "\"");
            std::string message;
            for (std::size_t i = 1; i < args.size(); i++)
                message += (i > 1 ? " " : "") + args[i];
            Logger::instance().log(it->second, "Console", message);
        });
    _commands.add("shortcuts", "shortcuts [reset]", "List the commands and their shortcuts, or reset them all",
        [this](const std::vector<std::string> &args) {
            Logger &log = Logger::instance();
            if (!args.empty() && args[0] == "reset") {
                _actions.resetAllShortcuts();
                log.info("Console", "Shortcuts reset to the defaults");
                return;
            }
            if (!args.empty())
                throw std::invalid_argument("unknown option \"" + args[0] + "\"");
            for (const auto &action : _actions.getActions()) {
                std::string keys;
                for (const ImGuiKeyChord chord : _actions.shortcuts(action->id))
                    keys += (keys.empty() ? "" : ", ") + ActionRegistry::shortcutText(chord);
                log.info("Console", action->id + "  " + (keys.empty() ? "-" : keys));
            }
        });
    _commands.add("bind", "bind <action-id> <shortcut>", "Add a shortcut, e.g. bind view.palette Ctrl+K",
        [this](const std::vector<std::string> &args) {
            if (args.size() != 2)
                throw std::invalid_argument("expected an action id and a shortcut");
            if (!_actions.has(args[0]))
                throw std::invalid_argument("unknown action \"" + args[0] + "\" (see \"shortcuts\")");
            const std::optional<ImGuiKeyChord> chord = ActionRegistry::parseShortcut(args[1]);
            if (!chord)
                throw std::invalid_argument("invalid shortcut \"" + args[1] + "\"");
            if (const std::optional<std::string> reason = ActionRegistry::reservedReason(*chord))
                throw std::invalid_argument(*reason);
            if (const Action *owner = _actions.findByShortcut(*chord); owner && owner->id != args[0])
                throw std::invalid_argument(args[1] + " is used by " + owner->id + "; unbind it first");
            _actions.addShortcut(args[0], *chord);
            Logger::instance().info("Console", ActionRegistry::shortcutText(*chord) + " -> " + args[0]);
        });
    _commands.add("unbind", "unbind <action-id> [shortcut]", "Remove one shortcut of a command, or all of them",
        [this](const std::vector<std::string> &args) {
            if (args.empty() || args.size() > 2)
                throw std::invalid_argument("expected an action id and optionally a shortcut");
            if (!_actions.has(args[0]))
                throw std::invalid_argument("unknown action \"" + args[0] + "\" (see \"shortcuts\")");
            if (args.size() == 1) {
                for (const ImGuiKeyChord chord : std::vector<ImGuiKeyChord>(_actions.shortcuts(args[0])))
                    _actions.removeShortcut(args[0], chord);
            } else {
                const std::optional<ImGuiKeyChord> chord = ActionRegistry::parseShortcut(args[1]);
                if (!chord || !_actions.removeShortcut(args[0], *chord))
                    throw std::invalid_argument(args[0] + " has no shortcut \"" + args[1] + "\"");
            }
            Logger::instance().info("Console", "Shortcut removed from " + args[0]);
        });
    _commands.add("workspace", "workspace <save|reset>", "Save the workspace now, or reset it",
        [this](const std::vector<std::string> &args) {
            if (args.size() == 1 && args[0] == "save") {
                _workspace->save();
                Logger::instance().info("Console", std::string("Workspace saved to ") + ImGui::GetIO().IniFilename);
            } else if (args.size() == 1 && args[0] == "reset") {
                resetWorkspace();
            } else {
                throw std::invalid_argument("expected save or reset");
            }
        });
    _commands.add("objects", "objects", "Count the objects of the scene", [this](const auto &) {
        const HierarchyModel &scene = _scene->hierarchy();
        std::size_t count = 0;
        for (HierarchyModel::ObjectId root : scene.roots())
            count += 1 + scene.countDescendants(root);
        Logger::instance().info("Console", std::to_string(count) + " objects in the scene");
    });
    _scene->registerCommands(_commands);
}

void Editor::registerPlayControls()
{
    _actions.add({.id = "game.play", .label = "Play", .callback = [this] { _play.play(); }, .shortcut = ImGuiKey_F5,
        .tooltip = "Run the game (resume when paused)", .isEnabled = [this] { return _play.getState() != PlayState::Playing; },
        .isChecked = [this] { return _play.getState() == PlayState::Playing; }});
    _actions.add({.id = "game.pause", .label = "Pause",
        .callback = [this] { _play.getState() == PlayState::Paused ? _play.play() : _play.pause(); }, .shortcut = ImGuiKey_F6,
        .tooltip = "Pause or resume the game", .isEnabled = [this] { return !_play.isStopped(); },
        .isChecked = [this] { return _play.getState() == PlayState::Paused; }});
    _actions.add({.id = "game.stop", .label = "Stop", .callback = [this] { _play.stop(); },
        .shortcut = ImGuiMod_Shift | ImGuiKey_F5, .tooltip = "Stop the game and go back to the edited scene",
        .isEnabled = [this] { return !_play.isStopped(); }});
    _actions.add({.id = "game.step", .label = "Step", .callback = [this] { _play.step(); }, .shortcut = ImGuiKey_F10,
        .tooltip = "Advance the game by one frame", .isEnabled = [this] { return _play.getState() != PlayState::Playing; }});

    _play.setOnStateChanged([this](PlayState state) {
        Logger &logger = Logger::instance();
        if (state == PlayState::Playing)
            logger.info("Game", "Playing");
        else if (state == PlayState::Paused)
            logger.info("Game", "Paused");
        else
            logger.info("Game", "Stopped: the scene is back to its state before Play");
        // Objects created during play no longer exist.
        if (_selection.has() && !_scene->hierarchy().exists(_selection.get()))
            _selection.clear();
    });

    _commands.add("play", "play", "Run or resume the game", [this](const auto &) { _play.play(); });
    _commands.add("pause", "pause", "Pause the game", [this](const auto &) { _play.pause(); });
    _commands.add("stop", "stop", "Stop the game and restore the scene", [this](const auto &) { _play.stop(); });
    _commands.add("step", "step [frames]", "Advance the game by one or more frames",
        [this](const std::vector<std::string> &args) {
            const int frames = args.empty() ? 1 : std::stoi(args[0]);
            if (frames < 1 || frames > 10000)
                throw std::invalid_argument("frames must be between 1 and 10000");
            for (int i = 0; i < frames; i++)
                _play.step();
        });
    _commands.add("timescale", "timescale <factor>", "Speed of the game (1 = normal, 0.5 = half speed)",
        [this](const std::vector<std::string> &args) {
            if (args.size() != 1)
                throw std::invalid_argument("expected a factor, e.g. 0.5");
            _play.setTimeScale(std::stod(args[0]));
            char text[48];
            std::snprintf(text, sizeof(text), "Time scale: x%.2g", _play.getTimeScale());
            Logger::instance().info("Console", text);
        });
}

void Editor::loadShortcuts()
{
    // Shared by both modes: shortcuts are a user preference, not a layout.
    static constexpr const char *ShortcutsFile = "shortcuts.ini";
    _actions.loadShortcuts(ShortcutsFile);
    _actions.setOnShortcutsChanged([this] {
        if (!_actions.saveShortcuts(ShortcutsFile))
            Logger::instance().error("Shortcuts", std::string("Cannot write ") + ShortcutsFile);
    });
}

void Editor::collectWorkspace(Workspace::Values &values) const
{
    const WindowState window = _gui.getWindowState();
    Workspace::setInt(values, "window.width", window.width);
    Workspace::setInt(values, "window.height", window.height);
    if (window.hasPosition) {
        Workspace::setInt(values, "window.x", window.x);
        Workspace::setInt(values, "window.y", window.y);
    }
    Workspace::setBool(values, "window.maximized", window.maximized);
    Workspace::setBool(values, "window.fullscreen", window.fullscreen);

    values["editor.theme"] = _gui.themes().current().name;
    Workspace::setBool(values, "editor.vsync", _gui.isVSync());
    Workspace::setBool(values, "editor.statusBar", _statusBar.isVisible());
    Workspace::setBool(values, "editor.performanceOverlay", _performanceOverlay.isVisible());

    for (const auto &panel : _panels.getPanels())
        Workspace::setBool(values, "panel." + panel->getName(), panel->isVisible());

    std::string levels;
    for (const LogLevel level : {LogLevel::Debug, LogLevel::Info, LogLevel::Success, LogLevel::Warning, LogLevel::Error})
        levels += _console->isLevelVisible(level) ? '1' : '0';
    values["console.levels"] = levels;
    std::string hidden;
    for (const std::string &category : _console->getHiddenCategories())
        hidden += (hidden.empty() ? "" : ",") + category;
    values["console.hiddenCategories"] = hidden;
    Workspace::setBool(values, "console.autoScroll", _console->isAutoScroll());
}

void Editor::applyWorkspace(const Workspace::Values &values)
{
    WindowState window;
    window.width = Workspace::getInt(values, "window.width", 0);
    window.height = Workspace::getInt(values, "window.height", 0);
    window.hasPosition = values.contains("window.x") && values.contains("window.y");
    window.x = Workspace::getInt(values, "window.x", 0);
    window.y = Workspace::getInt(values, "window.y", 0);
    window.maximized = Workspace::getBool(values, "window.maximized", false);
    window.fullscreen = Workspace::getBool(values, "window.fullscreen", false);
    _gui.applyWindowState(window);

    // A theme that no longer exists keeps the current one.
    _gui.themes().setTheme(Workspace::getString(values, "editor.theme", _gui.themes().current().name));
    _gui.setVSync(Workspace::getBool(values, "editor.vsync", _gui.isVSync()));
    _statusBar.setVisible(Workspace::getBool(values, "editor.statusBar", true));
    _performanceOverlay.setVisible(Workspace::getBool(values, "editor.performanceOverlay", true));

    // Panels added since the save keep their default visibility.
    for (const auto &panel : _panels.getPanels()) {
        const bool visible = Workspace::getBool(values, "panel." + panel->getName(), panel->isVisible());
        if (visible != panel->isVisible())
            panel->setVisible(visible);
    }

    const std::string levels = Workspace::getString(values, "console.levels", "");
    const LogLevel order[] = {LogLevel::Debug, LogLevel::Info, LogLevel::Success, LogLevel::Warning, LogLevel::Error};
    for (std::size_t i = 0; i < levels.size() && i < std::size(order); i++)
        _console->setLevelVisible(order[i], levels[i] == '1');
    std::set<std::string> hidden;
    const std::string list = Workspace::getString(values, "console.hiddenCategories", "");
    for (std::size_t start = 0; start < list.size();) {
        const std::size_t comma = list.find(',', start);
        const std::string category = list.substr(start, comma == std::string::npos ? std::string::npos : comma - start);
        if (!category.empty())
            hidden.insert(category);
        if (comma == std::string::npos)
            break;
        start = comma + 1;
    }
    _console->setHiddenCategories(std::move(hidden));
    _console->setAutoScroll(Workspace::getBool(values, "console.autoScroll", true));
}

void Editor::resetWorkspace()
{
    _dialogs.confirm({.title = "Reset workspace",
        .message = "Restore the default window size, panels, theme, layout and console filters?",
        .confirmLabel = "Reset",
        .destructive = true,
        .onConfirm = [this] {
            applyWorkspace(_defaultWorkspace);
            _layout.resetLayout();
            _notifications.info("Workspace reset");
        }});
}

void Editor::logStartupInfo() const
{
    Logger &logger = Logger::instance();
    const int sdl = SDL_GetVersion();
    char versions[128];
    std::snprintf(versions, sizeof(versions), "SDL %d.%d.%d, Dear ImGui %s", SDL_VERSIONNUM_MAJOR(sdl),
        SDL_VERSIONNUM_MINOR(sdl), SDL_VERSIONNUM_MICRO(sdl), IMGUI_VERSION);
    logger.info("Editor", _options.demo ? "Editor started in demo mode" : "Editor started");
    logger.info("Editor", _scene->description());
    logger.info("Editor", versions);
    logger.info("Renderer", std::string("Video driver: ") + SDL_GetCurrentVideoDriver() + ", renderer: " + _gui.getRendererName());
}

void Editor::reportNewErrors()
{
    const Logger &logger = Logger::instance();
    const std::size_t errorCount = logger.count(LogLevel::Error);
    if (errorCount == _knownErrorCount)
        return;
    const bool newErrors = errorCount > _knownErrorCount; // Fewer: the log was cleared
    _knownErrorCount = errorCount;
    if (!newErrors)
        return;

    // Errors from the editor itself already come from a notification, and
    // console errors are shown where the user is typing; the others (SDL,
    // engine, renderer...) get one so they are not missed.
    std::vector<LogEntry> entries;
    logger.copyEntries(entries);
    const LogEntry *latest = nullptr;
    int unseen = 0;
    for (const LogEntry &entry : entries) {
        if (entry.level != LogLevel::Error || entry.id <= _lastReportedError)
            continue;
        _lastReportedError = entry.id;
        if (entry.category == "Editor" || entry.category == "Console")
            continue;
        latest = &entry;
        unseen++;
    }
    if (!latest)
        return;
    // The notification is itself logged under "Editor", so it is not reported again.
    const std::string title = unseen == 1 ? "Error in " + latest->category : std::to_string(unseen) + " new errors";
    _notifications.error(title, latest->message);
}

void Editor::focusPanel(const std::string &name)
{
    Panel *panel = _panels.getPanel(name);
    if (!panel)
        return;
    if (panel->isVisible())
        ImGui::SetWindowFocus(name.c_str());
    else
        panel->setVisible(true); // Showing a panel also focuses it
}

void Editor::requestQuit()
{
    // Several triggers (shortcut, menu, window close) must not stack dialogs.
    if (_quitDialogOpen)
        return;
    _quitDialogOpen = true;
    _dialogs.confirm({.title = "Quit",
        .message = "Quit the editor? Unsaved changes will be lost.",
        .confirmLabel = "Quit",
        .destructive = true,
        .onConfirm = [this] { _gui.stop(); },
        .onCancel = [this] { _quitDialogOpen = false; }});
}

void Editor::run()
{
    _gui.run([this] { drawFrame(); });
}

void Editor::drawFrame()
{
    // The game advances before the panels draw it.
    _play.update(_gui.timer().getDeltaTime());

    // A dialog waits for an answer, or a new shortcut is being recorded:
    // editor shortcuts must not act meanwhile.
    if (!_dialogs.isOpen() && !_shortcutsPanel->isRecording())
        _actions.processShortcuts();
    reportNewErrors();
    _workspace->update();

    // Bars first: each one reserves its space, and the dockspace fills the rest.
    _menuBar.draw();
    _toolbar.draw();
    _statusBar.draw();
    _layout.draw();
    _panels.drawAll();

    // On top of the panels.
    _performanceOverlay.render();
    _palette.draw();
    _notifications.draw();
    _dialogs.draw();
}
