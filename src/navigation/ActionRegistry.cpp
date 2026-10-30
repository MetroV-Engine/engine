/*
    Created on: 30/03/2026
    Filename: ActionRegistry.cpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "ActionRegistry.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

Action &ActionRegistry::add(Action action)
{
    if (action.id.empty())
        throw std::invalid_argument("Action id must not be empty");
    if (has(action.id))
        throw std::invalid_argument("Action \"" + action.id + "\" is already registered");
    _actions.push_back(std::make_unique<Action>(std::move(action)));
    return *_actions.back();
}

Action *ActionRegistry::get(const std::string &id) const
{
    const auto it = std::find_if(
        _actions.begin(), _actions.end(), [&id](const auto &action) { return action->id == id; });
    return it != _actions.end() ? it->get() : nullptr;
}

bool ActionRegistry::execute(const std::string &id) const
{
    const Action *action = get(id);
    if (!action || !action->enabled() || !action->callback)
        return false;
    action->callback();
    return true;
}

void ActionRegistry::processShortcuts() const
{
    // While typing in a text field, shortcuts without Ctrl/Alt/Super (e.g. a
    // plain letter or Delete) belong to the field, not to the editor.
    const bool typing = ImGui::GetIO().WantTextInput;
    const ImGuiKeyChord commandMods = ImGuiMod_Ctrl | ImGuiMod_Alt | ImGuiMod_Super;

    for (const auto &action : _actions) {
        if (action->shortcut == ImGuiKey_None)
            continue;
        if (typing && !(action->shortcut & commandMods))
            continue;
        if (ImGui::Shortcut(action->shortcut, ImGuiInputFlags_RouteGlobal) && action->enabled() && action->callback)
            action->callback();
    }
}

std::string ActionRegistry::shortcutText(ImGuiKeyChord chord)
{
    if (chord == ImGuiKey_None)
        return "";
    std::string text;
    if (chord & ImGuiMod_Ctrl)
        text += "Ctrl+";
    if (chord & ImGuiMod_Shift)
        text += "Shift+";
    if (chord & ImGuiMod_Alt)
        text += "Alt+";
    if (chord & ImGuiMod_Super)
        text += "Super+";
    text += ImGui::GetKeyName(static_cast<ImGuiKey>(chord & ~ImGuiMod_Mask_));
    return text;
}
