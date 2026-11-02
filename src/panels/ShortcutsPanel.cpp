/*
    Created on: 30/03/2026
    Filename: ShortcutsPanel.cpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "ShortcutsPanel.hpp"
#include "components/Button.hpp"
#include "theme/ThemeManager.hpp"

#include <imgui_stdlib.h>

#include <algorithm>
#include <cctype>
#include <cfloat>
#include <utility>
#include <vector>

namespace {

using Shortcut = std::pair<const char *, const char *>; // keys, description

// Keys handled by panels or by ImGui itself, which are not editor actions.
const std::vector<std::pair<const char *, std::vector<Shortcut>>> &contextShortcuts()
{
    static const std::vector<std::pair<const char *, std::vector<Shortcut>>> groups = {
        {"Keyboard navigation",
            {{"Tab / Shift+Tab", "Next / previous widget"}, {"Arrows", "Move between widgets"},
                {"Space or Enter", "Activate the focused widget"}, {"Escape", "Cancel, close a popup or leave a field"},
                {"Ctrl+Tab", "Switch between panels (hold Ctrl, press Tab again)"}}},
        {"Hierarchy (when focused)",
            {{"Up / Down", "Select the previous / next object"}, {"Right / Left", "Expand / collapse, or go to the parent"},
                {"F2", "Rename the selected object"}, {"Delete", "Delete the selected object (asks first)"},
                {"Enter / Escape", "Confirm / cancel a rename"}}},
        {"Console command line",
            {{"Enter", "Run the command"}, {"Up / Down", "Browse the command history"}, {"Tab", "Complete the command name"},
                {"Escape", "Clear the line"}}},
        {"Dialogs", {{"Enter", "Confirm"}, {"Escape", "Cancel"}}},
        {"Number fields", {{"Drag", "Change the value"}, {"Ctrl+Click or double-click", "Type a value"}}},
        {"Text fields", {{"Ctrl+A", "Select all"}, {"Ctrl+C / Ctrl+X / Ctrl+V", "Copy / cut / paste"}, {"Ctrl+Z / Ctrl+Y", "Undo / redo"}}},
    };
    return groups;
}

bool matches(const std::string &filter, const std::string &a, const std::string &b)
{
    if (filter.empty())
        return true;
    const auto contains = [&](std::string text) {
        std::ranges::transform(text, text.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return text.find(filter) != std::string::npos;
    };
    return contains(a) || contains(b);
}

// One table per group; returns false if nothing matched.
bool drawGroup(const char *title, const std::vector<Shortcut> &rows, const std::string &filter)
{
    std::vector<const Shortcut *> visible;
    for (const Shortcut &row : rows)
        if (matches(filter, row.first, row.second) || matches(filter, title, ""))
            visible.push_back(&row);
    if (visible.empty())
        return false;

    ImGui::SeparatorText(title);
    if (ImGui::BeginTable(title, 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("Keys", ImGuiTableColumnFlags_WidthStretch, 0.4f);
        ImGui::TableSetupColumn("Description", ImGuiTableColumnFlags_WidthStretch, 0.6f);
        for (const Shortcut *row : visible) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(row->first);
            ImGui::TableNextColumn();
            ImGui::TextDisabled("%s", row->second);
        }
        ImGui::EndTable();
    }
    return true;
}

} // namespace

ShortcutsPanel::ShortcutsPanel(ActionRegistry &actions, DialogManager &dialogs, NotificationManager &notifications)
    : Panel("Keyboard Shortcuts"), _actions(actions), _dialogs(dialogs), _notifications(notifications)
{
    setInitialSize(ImVec2(640.0f, 640.0f));
}

std::string ShortcutsPanel::commandName(const Action &action)
{
    return ActionRegistry::categoryOf(action.id) + ": " + action.label;
}

void ShortcutsPanel::startRecording(const std::string &id)
{
    _recording = id;
}

void ShortcutsPanel::captureRecording()
{
    if (_recording.empty())
        return;
    if (ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {
        _recording.clear();
        return;
    }
    // The first keyboard key pressed (other than a modifier) ends the recording.
    for (int key = ImGuiKey_NamedKey_BEGIN; key <= ImGuiKey_Oem102; key++) {
        if ((key >= ImGuiKey_LeftCtrl && key <= ImGuiKey_RightSuper) || !ImGui::IsKeyPressed(static_cast<ImGuiKey>(key), false))
            continue;
        const std::string id = _recording;
        _recording.clear();
        assign(id, ImGui::GetIO().KeyMods | key);
        return;
    }
}

void ShortcutsPanel::assign(const std::string &id, ImGuiKeyChord chord)
{
    const Action *action = _actions.get(id);
    if (!action)
        return;
    const std::string text = ActionRegistry::shortcutText(chord);
    if (const std::optional<std::string> reason = ActionRegistry::reservedReason(chord)) {
        _notifications.warning("Cannot use " + text, *reason + ".");
        return;
    }

    const Action *owner = _actions.findByShortcut(chord);
    if (owner == action) {
        _notifications.info(text + " is already a shortcut of " + commandName(*action));
        return;
    }
    if (owner) {
        // Taken by another command: ask before moving it.
        const std::string ownerId = owner->id;
        _dialogs.confirm({.title = "Shortcut already used",
            .message = text + " is used by \"" + commandName(*owner) + "\".\nUse it for \"" + commandName(*action)
                + "\" instead?",
            .confirmLabel = "Reassign",
            .onConfirm = [this, id, ownerId, chord, text] {
                _actions.removeShortcut(ownerId, chord);
                if (_actions.addShortcut(id, chord))
                    _notifications.success("Shortcut reassigned", text + " -> " + commandName(*_actions.get(id)));
            }});
        return;
    }
    if (_actions.addShortcut(id, chord))
        _notifications.success("Shortcut added", text + " -> " + commandName(*action));
}

void ShortcutsPanel::drawContent()
{
    captureRecording();
    const Theme &theme = ThemeManager::active();

    // Header: search, reset all, help.
    const float resetWidth = ImGui::CalcTextSize("Reset All").x + ImGui::GetStyle().FramePadding.x * 2.0f;
    ImGui::SetNextItemWidth(-(resetWidth + ImGui::GetStyle().ItemSpacing.x));
    ImGui::InputTextWithHint("##Filter", "Search commands or shortcuts", &_filter);
    ImGui::SameLine();
    bool anyCustom = false;
    for (const auto &action : _actions.getActions())
        anyCustom |= !_actions.hasDefaultShortcuts(action->id);
    ImGui::BeginDisabled(!anyCustom);
    if (Button("Reset All", {}, Button::Variant::Secondary).render()) {
        _dialogs.confirm({.title = "Reset all shortcuts",
            .message = "Restore the default shortcut of every command? Your custom shortcuts will be lost.",
            .confirmLabel = "Reset All",
            .destructive = true,
            .onConfirm = [this] {
                _actions.resetAllShortcuts();
                _notifications.info("Shortcuts reset to the defaults");
            }});
    }
    ImGui::EndDisabled();
    ImGui::TextDisabled("Click + then press the keys. Click a shortcut to remove it.");

    std::string filter = _filter;
    std::ranges::transform(filter, filter.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    ImGui::BeginChild("##Shortcuts");
    bool any = false;

    // Every command, grouped by category in registration order.
    std::vector<std::string> categories;
    for (const auto &action : _actions.getActions()) {
        const std::string category = ActionRegistry::categoryOf(action->id);
        if (action->callback && std::ranges::find(categories, category) == categories.end())
            categories.push_back(category);
    }
    std::string removeId;
    ImGuiKeyChord removeChord = ImGuiKey_None;
    for (const std::string &category : categories) {
        std::vector<const Action *> rows;
        for (const auto &action : _actions.getActions()) {
            if (!action->callback || ActionRegistry::categoryOf(action->id) != category)
                continue;
            std::string keys;
            for (const ImGuiKeyChord chord : _actions.shortcuts(action->id))
                keys += ActionRegistry::shortcutText(chord) + " ";
            if (matches(filter, action->label, keys + action->shortcutHint) || matches(filter, category, ""))
                rows.push_back(action.get());
        }
        if (rows.empty())
            continue;
        any = true;

        ImGui::SeparatorText(category.c_str());
        if (!ImGui::BeginTable(category.c_str(), 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp))
            continue;
        ImGui::TableSetupColumn("Command", ImGuiTableColumnFlags_WidthStretch, 0.38f);
        ImGui::TableSetupColumn("Shortcuts", ImGuiTableColumnFlags_WidthStretch, 0.62f);
        ImGui::TableSetupColumn("Edit", ImGuiTableColumnFlags_WidthFixed);
        for (const Action *action : rows) {
            ImGui::PushID(action->id.c_str());
            ImGui::TableNextRow();

            ImGui::TableNextColumn();
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(action->label.c_str());
            const bool custom = !_actions.hasDefaultShortcuts(action->id);
            if (custom) {
                ImGui::SameLine();
                ImGui::TextColored(theme.colors.accent, "*");
                ImGui::SetItemTooltip("Changed from the default (%s)",
                    action->shortcut != ImGuiKey_None ? ActionRegistry::shortcutText(action->shortcut).c_str() : "none");
            }

            ImGui::TableNextColumn();
            const std::vector<ImGuiKeyChord> chords = _actions.shortcuts(action->id);
            if (_recording == action->id) {
                ImGui::AlignTextToFramePadding();
                ImGui::TextColored(theme.colors.warning, "Press a shortcut... (Esc to cancel)");
            } else if (chords.empty()) {
                ImGui::AlignTextToFramePadding();
                if (!action->shortcutHint.empty())
                    ImGui::TextDisabled("%s (in the panel)", action->shortcutHint.c_str());
                else
                    ImGui::TextDisabled("None");
            }
            for (std::size_t i = 0; _recording != action->id && i < chords.size(); i++) {
                if (i > 0)
                    ImGui::SameLine();
                const std::string label = ActionRegistry::shortcutText(chords[i]) + "##" + std::to_string(i);
                if (ImGui::SmallButton(label.c_str())) {
                    removeId = action->id;
                    removeChord = chords[i];
                }
                ImGui::SetItemTooltip("Click to remove");
            }

            ImGui::TableNextColumn();
            if (ImGui::SmallButton("+"))
                startRecording(action->id);
            ImGui::SetItemTooltip("Add a shortcut");
            ImGui::SameLine();
            ImGui::BeginDisabled(!custom);
            if (ImGui::SmallButton("Reset"))
                _actions.resetShortcuts(action->id);
            ImGui::EndDisabled();
            ImGui::PopID();
        }
        ImGui::EndTable();
    }

    // Keys handled by the panels themselves; listed for reference.
    for (const auto &[title, rows] : contextShortcuts())
        any |= drawGroup(title, rows, filter);

    if (!any)
        ImGui::TextDisabled("Nothing matches \"%s\".", _filter.c_str());
    ImGui::EndChild();

    // Applied after drawing the list it belongs to.
    if (!removeId.empty() && _actions.removeShortcut(removeId, removeChord))
        _notifications.info("Shortcut removed", ActionRegistry::shortcutText(removeChord) + " from "
                + commandName(*_actions.get(removeId)));
}
