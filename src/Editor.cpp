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

#include <imgui.h>

#include <string>

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
    connectFeedback();
    _gui.setCloseRequestHandler([this] { requestQuit(); });
    _console->log(ConsolePanel::Level::Info, "Editor started");
}

void Editor::registerPanels()
{
    using Area = EditorLayout::Area;

    _hierarchy = &_panels.registerPanel<HierarchyPanel>(_scene, _selection, _notifications, _dialogs);
    _layout.setDefaultArea("Hierarchy", Area::Left);

    _panels.registerPanel<ViewportPanel>(_performanceOverlay);
    _layout.setDefaultArea("Viewport", Area::Center);

    _panels.registerPanel<InspectorPanel>(_scene, _selection);
    _layout.setDefaultArea("Inspector", Area::Right);
    _panels.registerPanel<SettingsPanel>(_gui, _performanceOverlay);
    _layout.setDefaultArea("Settings", Area::Right);

    _console = &_panels.registerPanel<ConsolePanel>();
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
    _statusBar.addItem([this] { ImGui::Text("%.0f FPS", _gui.timer().getFps()); });
    _statusBar.addItem([this] { ImGui::TextUnformatted(_gui.themes().current().name.c_str()); });
}

void Editor::connectFeedback()
{
    // Every notification is also kept in the console and shown in the status bar.
    _notifications.setOnNotify(
        [this](NotificationManager::Level level, const std::string &title, const std::string &message) {
            const ConsolePanel::Level consoleLevel = level == NotificationManager::Level::Success ? ConsolePanel::Level::Success
                : level == NotificationManager::Level::Warning ? ConsolePanel::Level::Warning
                : level == NotificationManager::Level::Error   ? ConsolePanel::Level::Error
                                                                : ConsolePanel::Level::Info;
            _console->log(consoleLevel, message.empty() ? title : title + ": " + message);
            _statusBar.setMessage(title);
        });
    _gui.themes().setOnChange([this](const Theme &theme) { _notifications.info("Theme changed", theme.name); });
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
