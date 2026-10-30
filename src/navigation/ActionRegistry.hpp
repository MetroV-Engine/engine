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

#include "Action.hpp"

#include <memory>
#include <string>
#include <vector>

// Owns every editor Action and triggers them from their keyboard shortcuts.
class ActionRegistry
{
public:
    // Throws std::invalid_argument if the id is empty or already registered.
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

    // Human-readable shortcut, e.g. "Ctrl+Shift+S". Empty for ImGuiKey_None.
    static std::string shortcutText(ImGuiKeyChord chord);

private:
    std::vector<std::unique_ptr<Action>> _actions;
};
