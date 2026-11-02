/*
    Created on: 30/03/2026
    Filename: Editor.hpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#pragma once

#include "GuiContext.hpp"
#include "PanelManager.hpp"
#include "components/PerformanceOverlay.hpp"
#include "feedback/DialogManager.hpp"
#include "feedback/NotificationManager.hpp"
#include "layout/EditorLayout.hpp"
#include "layout/StatusBar.hpp"
#include "log/CommandRegistry.hpp"
#include "navigation/ActionRegistry.hpp"
#include "navigation/CommandPalette.hpp"
#include "navigation/MenuBar.hpp"
#include "navigation/Toolbar.hpp"
#include "scene/DemoScene.hpp"
#include "scene/Selection.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

class ConsolePanel;
class HierarchyPanel;
class ShortcutsPanel;

// The editor application: assembles the panels, actions, menus, toolbar,
// status bar, dockspace and feedback systems into the main interface, and
// draws them each frame in the right order.
//
//   +---------------------------------------------------+
//   | Menu bar                                          |
//   | Toolbar                                           |
//   +-----------+-----------------------+---------------+
//   | Hierarchy |       Viewport        | Inspector     |
//   |           |                       | Settings      |
//   |           +-----------------------+               |
//   |           | Console | Buttons ... |               |
//   +-----------+-----------------------+---------------+
//   | Status bar                                        |
//   +---------------------------------------------------+
struct EditorOptions
{
    // Demo mode: loads the demo level and adds the widget demo panels
    // (Buttons, Inputs, Dear ImGui Demo). Otherwise the editor starts clean,
    // with an empty scene and only the editor panels.
    bool demo = false;
};

class Editor
{
public:
    using Options = EditorOptions;

    explicit Editor(GuiContext &gui, Options options = {});

    // Runs until the user quits.
    void run();

private:
    GuiContext &_gui;
    Options _options;
    PerformanceOverlay _performanceOverlay;
    NotificationManager _notifications;
    DialogManager _dialogs;
    // Scene shown in the Hierarchy: a demo level until the ECS is connected.
    DemoScene _scene;
    Selection _selection;
    CommandRegistry _commands;
    PanelManager _panels;
    ActionRegistry _actions;
    CommandPalette _palette;
    MenuBar _menuBar;
    Toolbar _toolbar;
    StatusBar _statusBar;
    EditorLayout _layout;
    ConsolePanel *_console = nullptr;
    HierarchyPanel *_hierarchy = nullptr;
    ShortcutsPanel *_shortcutsPanel = nullptr;
    bool _quitDialogOpen = false;
    // Errors already reported with a notification (by Logger entry id / count).
    std::uint64_t _lastReportedError = 0;
    std::size_t _knownErrorCount = 0;

    // Panels listed in View > Panels and restored by Reset Layout.
    std::vector<std::string> _layoutPanels;

    void registerPanels();
    void registerActions();
    void buildMenuBar();
    void buildToolbar();
    void buildStatusBar();
    void registerCommands();
    void connectFeedback();
    // Loads the user's shortcuts and saves them whenever they change.
    void loadShortcuts();
    void logStartupInfo() const;
    // Shows a notification when the engine or a system logs a new error.
    void reportNewErrors();
    void drawFrame();
    // Shows the panel (if hidden) and gives it the keyboard focus.
    void focusPanel(const std::string &name);
    // Asks for confirmation, then quits.
    void requestQuit();
};
