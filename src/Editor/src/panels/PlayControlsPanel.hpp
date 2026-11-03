/*
    Created on: 30/03/2026
    Filename: PlayControlsPanel.hpp
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
#include "navigation/ActionRegistry.hpp"
#include "play/PlayController.hpp"

// Bar of large colored buttons to run the game: Play (green), Pause
// (orange), Stop (red) and Step, with the play state, a timer and the speed.
// Docked above the console by default, without a tab. The buttons run the
// same actions as the Game menu and the shortcuts (F5, F6, Shift+F5, F10).
class PlayControlsPanel : public Panel
{
public:
    PlayControlsPanel(const ActionRegistry &actions, PlayController &play);

protected:
    void drawContent() override;
    // Floating, the title bar is added: keep room for the row of buttons.
    ImVec2 minimumSize() const override;

private:
    enum class Icon
    {
        Play,
        Pause,
        Stop,
        Step,
    };

    const ActionRegistry &_actions;
    PlayController &_play;

    // A filled button with a drawn icon and a label; returns true when clicked.
    bool controlButton(const char *actionId, Icon icon, const ImVec4 &color, float height) const;
    static float buttonWidth(const char *label, float height);
    static void drawIcon(ImDrawList *drawList, Icon icon, ImVec2 center, float size, ImU32 color);
};
