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

#include "panels/Panel/Panel.hpp"
#include "navigation/ActionRegistry/ActionRegistry.hpp"
#include "scene/PlayController/PlayController.hpp"

#include <string>

// Bar of large colored buttons to run the game: Play (green), Pause
// (orange), Stop (red) and Step, with the play state, a timer and the speed.
// Docked above the console by default, without a tab. The buttons run the
// same actions as the Game menu and the shortcuts (F5, F6, Shift+F5, F10).
// When the bar is too narrow, the buttons lose their label (named in their
// tooltip), then the state name and the "Speed" caption are left out, then
// the timer (the lit button still shows the state).
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

    // What the row shows: the parts that do not fit are dropped, in this order.
    struct RowParts
    {
        bool labels = true;
        bool captions = true;
        // The state dot and the timer.
        bool status = true;
    };

    const ActionRegistry &_actions;
    PlayController &_play;

    // A filled button with a drawn icon, and its label if asked; returns true when clicked.
    bool controlButton(const char *actionId, Icon icon, const ImVec4 &color, float height, bool label) const;
    static float buttonWidth(const char *label, float height);
    static void drawIcon(ImDrawList *drawList, Icon icon, ImVec2 center, float size, ImU32 color);
    // "Editing", or the state and the timer; only the timer without captions.
    static std::string statusText(PlayState state, double time, bool captions);
    // The widest status, so the row never moves when the state or the timer changes.
    static float statusWidth(bool captions);
    float rowWidth(const RowParts &parts, float height) const;
};
