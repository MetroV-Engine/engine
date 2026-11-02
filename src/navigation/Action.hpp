/*
    Created on: 30/03/2026
    Filename: Action.hpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#pragma once

#include <imgui.h>

#include <functional>
#include <string>

// An editor command (Save, Quit, Toggle Fullscreen...) defined once and shared
// by the menu bar, the toolbar and keyboard shortcuts, so they always behave
// and display the same way.
struct Action
{
    // Unique identifier, e.g. "file.quit" or "view.panel.Settings".
    std::string id;
    // Text shown in menus and as toolbar button.
    std::string label;
    std::function<void()> callback;
    // Optional default keyboard shortcut, e.g. ImGuiMod_Ctrl | ImGuiKey_Q.
    // Users can change, add or remove shortcuts: the current ones are kept by
    // the ActionRegistry (see ActionRegistry::shortcuts()).
    ImGuiKeyChord shortcut = ImGuiKey_None;
    // Optional hover text in the toolbar.
    std::string tooltip;
    // Optional: the action is greyed out and cannot run while this returns false.
    std::function<bool()> isEnabled;
    // Optional: makes the action a toggle, shown with a check mark (menu) or
    // highlighted (toolbar) while this returns true.
    std::function<bool()> isChecked;

    // Shortcut shown in menus for keys handled by a panel itself, not by the
    // registry (e.g. "F2" for rename, active only when the Hierarchy has focus).
    std::string shortcutHint;

    bool enabled() const { return !isEnabled || isEnabled(); }
    bool checkable() const { return static_cast<bool>(isChecked); }
    bool checked() const { return isChecked && isChecked(); }
};
