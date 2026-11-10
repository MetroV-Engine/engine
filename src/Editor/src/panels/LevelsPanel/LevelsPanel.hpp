/*
    Created on: 10/11/2026
    Filename: LevelsPanel.hpp
    Description: Panel to create, open, save, rename and delete the levels of the project

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#pragma once

#include "panels/Panel/Panel.hpp"
#include "scene/LevelManager/LevelManager.hpp"
#include "scene/PlayController/PlayController.hpp"
#include "scene/Selection.hpp"
#include "ui/DialogManager/DialogManager.hpp"
#include "ui/NotificationManager/NotificationManager.hpp"

#include <functional>
#include <string>
#include <vector>

// The levels of the project (see LevelManager): the open one with Save, Save
// As and New, then the levels folder, to open a level (double click), rename
// or delete it (right click). Unsaved changes are only discarded after a
// confirmation. Only while the game is stopped: playing changes the scene.
class LevelsPanel : public Panel
{
public:
    // Everything given must outlive the panel.
    LevelsPanel(LevelManager &levels, const PlayController &play, Selection &selection, NotificationManager &notifications,
        DialogManager &dialogs);

    // Saves the open level, or asks for the name of a new one (Ctrl+S).
    void save();

protected:
    void drawContent() override;

private:
    // What the name field is for.
    enum class Naming
    {
        None,
        New,
        SaveAs,
        Rename,
    };

    LevelManager &_levels;
    const PlayController &_play;
    Selection &_selection;
    NotificationManager &_notifications;
    DialogManager &_dialogs;

    Naming _naming = Naming::None;
    std::string _name;
    // The level being renamed.
    std::string _renamed;
    bool _focusName = false;
    // The levels folder, read again at most every second.
    std::vector<std::string> _names;
    double _nextRefresh = 0.0;

    void startNaming(Naming naming, std::string name);
    void drawNameField();
    void confirmName();
    void drawList();
    // Runs change; a refusal becomes a notification titled failure.
    bool apply(const std::string &failure, const std::function<void()> &change);
    // Runs then, after a confirmation if the scene has unsaved changes.
    void discardChanges(const std::string &title, std::function<void()> then);
    void open(const std::string &name);
    void create(const std::string &name);
    void requestDelete(const std::string &name);
};
