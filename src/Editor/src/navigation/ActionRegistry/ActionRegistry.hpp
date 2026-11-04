/*
    Created on: 30/03/2026
    Filename: ActionRegistry.hpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#pragma once

#include "navigation/Action.hpp"

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

// Owns every editor Action and triggers them from their keyboard shortcuts.
// Each action has a list of shortcuts (zero, one or more), starting from its
// default Action::shortcut; users can add, remove and reset them, and save
// them to a file.
class ActionRegistry
{
public:
    // Throws std::invalid_argument if the id is empty or already registered.
    // A default shortcut already used by another action is logged as a warning.
    // The returned reference stays valid for the registry's lifetime.
    Action &add(Action action);

    // Returns nullptr if no action has this id.
    Action *get(const std::string &id) const;
    bool has(const std::string &id) const { return get(id) != nullptr; }

    // Runs the action if it exists and is enabled; returns whether it ran.
    bool execute(const std::string &id) const;

    // Runs the actions whose shortcut was pressed this frame. Call once per
    // frame, between beginFrame() and present().
    void processShortcuts() const;

    const std::vector<std::unique_ptr<Action>> &getActions() const { return _actions; }

    // ---- Shortcuts

    // Current shortcuts of an action (empty if none or unknown id).
    const std::vector<ImGuiKeyChord> &shortcuts(const std::string &id) const;
    // Returns false if the id is unknown, the chord invalid or already bound
    // to this action. Does not check other actions: see findByShortcut().
    bool addShortcut(const std::string &id, ImGuiKeyChord chord);
    bool removeShortcut(const std::string &id, ImGuiKeyChord chord);
    // Restores the default shortcut of one action, or of all of them.
    void resetShortcuts(const std::string &id);
    void resetAllShortcuts();
    // Whether the action still has exactly its default shortcut.
    bool hasDefaultShortcuts(const std::string &id) const;
    // The action this chord triggers, or nullptr.
    const Action *findByShortcut(ImGuiKeyChord chord) const;
    // Called after any change of shortcuts (e.g. to save them).
    void setOnShortcutsChanged(std::function<void()> callback) { _onShortcutsChanged = std::move(callback); }

    // Saves the shortcuts that differ from the defaults, one line per action:
    //   view.palette=Ctrl+Shift+P,F5
    //   view.console=            (no shortcut)
    bool saveShortcuts(const std::string &path) const;
    // Applies a file written by saveShortcuts(). Unknown actions and invalid
    // shortcuts are logged and skipped. A missing file is not an error.
    void loadShortcuts(const std::string &path);

    // First shortcut of the action as text, or its shortcutHint if it has none.
    std::string shortcutLabel(const Action &action) const;

    // Human-readable shortcut, e.g. "Ctrl+Shift+S". Empty for ImGuiKey_None.
    static std::string shortcutText(ImGuiKeyChord chord);
    // Inverse of shortcutText ("ctrl+shift+s" works too).
    static std::optional<ImGuiKeyChord> parseShortcut(const std::string &text);
    // Keys kept for keyboard navigation and text editing (Tab, Enter, Escape,
    // Space, arrows, Backspace without modifier...). Returns the reason, or
    // nothing if the chord can be used as a shortcut.
    static std::optional<std::string> reservedReason(ImGuiKeyChord chord);
    // Display group of an action, from its id: "file.quit" -> "File",
    // "view.panel.Console" -> "Panel", "view.theme.Dark" -> "Theme".
    static std::string categoryOf(const std::string &id);

private:
    std::vector<std::unique_ptr<Action>> _actions;
    std::unordered_map<std::string, std::vector<ImGuiKeyChord>> _shortcuts;
    std::function<void()> _onShortcutsChanged;

    std::vector<ImGuiKeyChord> defaultShortcuts(const Action &action) const;
    void notifyChanged() const;
};
