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

#include "core/AssetLibrary/AssetLibrary.hpp"
#include "core/GuiContext/GuiContext.hpp"
#include "core/PanelManager/PanelManager.hpp"
#include "ui/PerformanceOverlay/PerformanceOverlay.hpp"
#include "ui/DialogManager/DialogManager.hpp"
#include "ui/NotificationManager/NotificationManager.hpp"
#include "layout/EditorLayout/EditorLayout.hpp"
#include "layout/StatusBar/StatusBar.hpp"
#include "layout/Workspace/Workspace.hpp"
#include "core/CommandRegistry/CommandRegistry.hpp"
#include "navigation/ActionRegistry/ActionRegistry.hpp"
#include "navigation/CommandPalette/CommandPalette.hpp"
#include "navigation/MenuBar/MenuBar.hpp"
#include "navigation/Toolbar/Toolbar.hpp"
#include "panels/EntityDropTarget/EntityDropTarget.hpp"
#include "scene/LevelManager/LevelManager.hpp"
#include "scene/PlayController/PlayController.hpp"
#include "scene/SceneBackend.hpp"
#include "scene/Selection.hpp"
#include "scene/ViewportGrid.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace ECS {
class Registry;
class SystemManager;
} // namespace ECS

class ConsolePanel;
class HierarchyPanel;
class LevelsPanel;
class ShortcutsPanel;
class ViewportPanel;

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
//   +-----------+-----------------------+ Levels        |
//   | Entity    | Console | Assets ...  |               |
//   | Palette   |                       |               |
//   +-----------+-----------------------+---------------+
//   | Status bar                                        |
//   +---------------------------------------------------+
//
// What it edits and plays is a SceneBackend: an ECS world given by the
// engine or a game, or the built-in demo scene.
struct EditorOptions
{
    // Demo mode: adds the widget demo panels (Buttons, Inputs, Dear ImGui
    // Demo) and, for the built-in demo scene, loads the demo level.
    // Otherwise only the editor panels are shown.
    bool demo = false;
};

class Editor
{
public:
    using Options = EditorOptions;

    // Editor of an ECS world: the Hierarchy lists its entities, the Inspector
    // edits their reflected components, Play runs the systems on it and Stop
    // restores it (see EcsBackend). world and systems must outlive the editor.
    Editor(GuiContext &gui, ECS::Registry &world, ECS::SystemManager &systems, Options options = {});
    // Editor of the built-in demo scene, without the engine: empty, or the
    // demo level with options.demo.
    explicit Editor(GuiContext &gui, Options options = {});
    // Editor of any scene.
    Editor(GuiContext &gui, std::unique_ptr<SceneBackend> scene, Options options = {});
    // Stops the game first, so the scene is left as it was edited.
    ~Editor();

    Editor(const Editor &) = delete;
    Editor &operator=(const Editor &) = delete;

    // Draws the game in the Viewport, replacing the scene's own renderer
    // (until the engine renders into it, e.g. a game's debug drawing). The
    // view it returns lets the mouse place objects there.
    void setViewportRenderer(SceneBackend::Renderer renderer);

    // Adds a template to the Entity Palette, after those of the scene.
    void addEntityTemplate(EntityTemplate entityTemplate);

    // Runs until the user quits.
    void run();

private:
    GuiContext &_gui;
    Options _options;
    PerformanceOverlay _performanceOverlay;
    NotificationManager _notifications;
    DialogManager _dialogs;
    // What the Hierarchy and Inspector show and Play runs.
    std::unique_ptr<SceneBackend> _scene;
    Selection _selection;
    PlayController _play;
    // Snapping of the Viewport's mouse edits, set in Settings, saved with the workspace.
    ViewportGrid _grid;
    // Only ever appended to: drag and drop payloads are indices into it.
    std::vector<EntityTemplate> _entityTemplates;
    EntityDropTarget _entityDropTarget;
    // The levels folder, opened into the scene and saved from it.
    LevelManager _levels;
    // The assets folder, shown by the Asset Browser.
    AssetLibrary _assets;
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
    LevelsPanel *_levelsPanel = nullptr;
    ShortcutsPanel *_shortcutsPanel = nullptr;
    ViewportPanel *_viewport = nullptr;
    bool _quitDialogOpen = false;
    // Errors already reported with a notification (by Logger entry id / count).
    std::uint64_t _lastReportedError = 0;
    std::size_t _knownErrorCount = 0;
    // State of a fresh editor, used by Reset Workspace.
    Workspace::Values _defaultWorkspace;
    // Last member: destroyed first, while what it saves still exists.
    std::unique_ptr<Workspace> _workspace;

    // Panels listed in View > Panels and restored by Reset Layout.
    std::vector<std::string> _layoutPanels;

    void registerPanels();
    void registerActions();
    void buildMenuBar();
    void buildToolbar();
    void buildStatusBar();
    void registerCommands();
    // The "level" console command: list, new, open, save, rename, delete.
    void levelCommand(const std::vector<std::string> &args);
    void registerPlayControls();
    void connectFeedback();
    // Loads the user's shortcuts and saves them whenever they change.
    void loadShortcuts();
    // Saves / restores window geometry, open panels and preferences.
    void collectWorkspace(Workspace::Values &values) const;
    void applyWorkspace(const Workspace::Values &values);
    void resetWorkspace();
    void logStartupInfo() const;
    // Shows a notification when the engine or a system logs a new error.
    void reportNewErrors();
    void drawFrame();
    // Shows the panel (if hidden) and gives it the keyboard focus.
    void focusPanel(const std::string &name);
    // Asks for confirmation, then quits.
    void requestQuit();
};
