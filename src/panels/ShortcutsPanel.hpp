/*
    Created on: 30/03/2026
    Filename: ShortcutsPanel.hpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#pragma once

#include "Panel.hpp"
#include "feedback/DialogManager.hpp"
#include "feedback/NotificationManager.hpp"
#include "navigation/ActionRegistry.hpp"

#include <string>

// Keyboard shortcuts editor (F1): every editor command with its shortcuts,
// which can be added (click +, then press the keys), removed (click the
// shortcut) or reset to the default. A shortcut already used by another
// command is reassigned only after confirmation. Keys handled by the panels
// themselves (Hierarchy, Console, navigation...) are listed for reference.
class ShortcutsPanel : public Panel
{
public:
    ShortcutsPanel(ActionRegistry &actions, DialogManager &dialogs, NotificationManager &notifications);

    // While a shortcut is being recorded, the editor must not run shortcuts
    // (pressing Ctrl+Q to bind it must not quit).
    bool isRecording() const { return !_recording.empty(); }

protected:
    void drawContent() override;

private:
    ActionRegistry &_actions;
    DialogManager &_dialogs;
    NotificationManager &_notifications;
    std::string _filter;
    // Id of the action waiting for a key press, empty if none.
    std::string _recording;

    void startRecording(const std::string &id);
    // Ends the recording on the first key press (Escape cancels).
    void captureRecording();
    // Binds chord to the action, after checking reserved keys and conflicts.
    void assign(const std::string &id, ImGuiKeyChord chord);
    static std::string commandName(const Action &action);
};
