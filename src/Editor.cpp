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
#include "panels/ButtonsPanel.hpp"
#include "panels/ConsolePanel.hpp"
#include "panels/HierarchyPanel.hpp"
#include "panels/ImGuiDemoPanel.hpp"
#include "panels/InputsPanel.hpp"
#include "panels/InspectorPanel.hpp"
#include "panels/SettingsPanel.hpp"
#include "panels/ViewportPanel.hpp"
#include "log/Logger.hpp"

#include <imgui.h>

#include <algorithm>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

// Panels listed in View > Panels, in this order.
const char *const EditorPanels[] = {"Hierarchy", "Viewport", "Inspector", "Console", "Settings", "Buttons", "Inputs"};

} // namespace

Editor::Editor(GuiContext &gui)
    : _gui(gui), _performanceOverlay(gui.timer()), _menuBar(_actions), _toolbar(_actions)
{
    registerPanels();
    registerActions();
    buildMenuBar();
    buildToolbar();
    buildStatusBar();
    registerCommands();
    connectFeedback();
    _gui.setCloseRequestHandler([this] { requestQuit(); });
    logStartupInfo();
}

void Editor::registerPanels()
{
    using Area = EditorLayout::Area;

    _hierarchy = &_panels.registerPanel<HierarchyPanel>(_scene, _selection, _notifications, _dialogs);
    _layout.setDefaultArea("Hierarchy", Area::Left);

    _panels.registerPanel<ViewportPanel>(_performanceOverlay);
    _layout.setDefaultArea("Viewport", Area::Center);

    _panels.registerPanel<InspectorPanel>(_scene, _scene, _selection, _notifications);
    _layout.setDefaultArea("Inspector", Area::Right);
    _panels.registerPanel<SettingsPanel>(_gui, _performanceOverlay);
    _layout.setDefaultArea("Settings", Area::Right);

    _console = &_panels.registerPanel<ConsolePanel>(_commands);
    _layout.setDefaultArea("Console", Area::Bottom);
    // Widget demos, next to the console.
    _panels.registerPanel<ButtonsPanel>(_notifications, _dialogs, [this] { requestQuit(); });
    _layout.setDefaultArea("Buttons", Area::Bottom);
    _panels.registerPanel<InputsPanel>();
    _layout.setDefaultArea("Inputs", Area::Bottom);

    // Floating windows, hidden until opened from the Help menu.
    _panels.registerPanel<ImGuiDemoPanel>().setVisible(false);
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
    _actions.add({.id = "edit.rename", .label = "Rename", .callback = [this] { _hierarchy->startRename(_selection.get()); },
        .isEnabled = [this] { return _selection.has(); }});
    _actions.add({.id = "edit.delete", .label = "Delete", .callback = [this] { _hierarchy->requestDelete(_selection.get()); },
        .isEnabled = [this] { return _selection.has(); }});

    _actions.add({.id = "view.console", .label = "Developer Console",
        .callback = [this] {
            _panels.setVisible("Console", true);
            ImGui::SetWindowFocus("Console");
            _console->focusCommandLine();
        },
        .shortcut = ImGuiKey_GraveAccent, .tooltip = "Open the console and type a command"});

    _actions.add({.id = "view.layout.reset", .label = "Reset Layout",
        .callback = [this] {
            for (const char *name : EditorPanels)
                _panels.setVisible(name, true);
            _layout.resetLayout();
            _notifications.info("Layout reset");
        },
        .tooltip = "Restore the default panel arrangement"});
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

    for (const char *name : EditorPanels)
        _menuBar.addAction("View/Panels", std::string("view.panel.") + name);
    _menuBar.addAction("View", "view.console");
    _menuBar.addAction("View", "view.layout.reset");
    _menuBar.addSeparator("View");
    _menuBar.addAction("View", "view.fullscreen");
    _menuBar.addAction("View", "view.performance");
    _menuBar.addAction("View", "view.statusbar");
    for (const std::string &name : _gui.themes().getThemeNames())
        _menuBar.addAction("View/Theme", "view.theme." + name);

    _menuBar.addAction("Help", "view.panel.Dear ImGui Demo");
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
    _commands.add("objects", "objects", "Count the objects of the scene", [this](const auto &) {
        std::size_t count = 0;
        for (HierarchyModel::ObjectId root : _scene.roots())
            count += 1 + _scene.countDescendants(root);
        Logger::instance().info("Console", std::to_string(count) + " objects in the scene");
    });
}

void Editor::logStartupInfo() const
{
    Logger &logger = Logger::instance();
    const int sdl = SDL_GetVersion();
    char versions[128];
    std::snprintf(versions, sizeof(versions), "SDL %d.%d.%d, Dear ImGui %s", SDL_VERSIONNUM_MAJOR(sdl),
        SDL_VERSIONNUM_MINOR(sdl), SDL_VERSIONNUM_MICRO(sdl), IMGUI_VERSION);
    logger.info("Editor", "Editor started");
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
    // A dialog waits for an answer: editor shortcuts must not act behind it.
    if (!_dialogs.isOpen())
        _actions.processShortcuts();
    reportNewErrors();

    // Bars first: each one reserves its space, and the dockspace fills the rest.
    _menuBar.draw();
    _toolbar.draw();
    _statusBar.draw();
    _layout.draw();
    _panels.drawAll();

    // On top of the panels.
    _performanceOverlay.render();
    _notifications.draw();
    _dialogs.draw();
}
