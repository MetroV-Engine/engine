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
#include "navigation/ActionRegistry.hpp"
#include "navigation/MenuBar.hpp"
#include "navigation/Toolbar.hpp"

class ConsolePanel;

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
    PanelManager _panels;
    ActionRegistry _actions;
    MenuBar _menuBar;
    Toolbar _toolbar;
    StatusBar _statusBar;
    EditorLayout _layout;
    ConsolePanel *_console = nullptr;
    bool _quitDialogOpen = false;

    void registerPanels();
    void registerActions();
    void buildMenuBar();
    void buildToolbar();
    void buildStatusBar();
    void connectFeedback();
    void drawFrame();
    // Asks for confirmation, then quits.
    void requestQuit();
};
