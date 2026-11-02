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
#include "navigation/MenuBar.hpp"
#include "navigation/Toolbar.hpp"
#include "scene/DemoScene.hpp"
#include "scene/Selection.hpp"

#include <cstddef>
#include <cstdint>

class ConsolePanel;
class HierarchyPanel;

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
class Editor
{
public:
    explicit Editor(GuiContext &gui);

    // Runs until the user quits.
    void run();

private:
    GuiContext &_gui;
    PerformanceOverlay _performanceOverlay;
    NotificationManager _notifications;
    DialogManager _dialogs;
    // Scene shown in the Hierarchy: a demo level until the ECS is connected.
    DemoScene _scene;
    Selection _selection;
    CommandRegistry _commands;
    PanelManager _panels;
    ActionRegistry _actions;
    MenuBar _menuBar;
    Toolbar _toolbar;
    StatusBar _statusBar;
    EditorLayout _layout;
    ConsolePanel *_console = nullptr;
    HierarchyPanel *_hierarchy = nullptr;
    bool _quitDialogOpen = false;
    // Errors already reported with a notification (by Logger entry id / count).
    std::uint64_t _lastReportedError = 0;
    std::size_t _knownErrorCount = 0;

    void registerPanels();
    void registerActions();
    void buildMenuBar();
    void buildToolbar();
    void buildStatusBar();
    void registerCommands();
    void connectFeedback();
    void logStartupInfo() const;
    // Shows a notification when the engine or a system logs a new error.
    void reportNewErrors();
    void drawFrame();
    // Asks for confirmation, then quits.
    void requestQuit();
};
