/*
    Created on: 30/03/2026
    Filename: Toolbar.cpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "Toolbar.hpp"
#include "ui/ThemeManager/ThemeManager.hpp"

// BeginViewportSideBar() is the only way to reserve space at the top of the
// viewport like the main menu bar does (no public equivalent yet).
#include <imgui_internal.h>

#include <stdexcept>

void Toolbar::addAction(const std::string &actionId)
{
    if (!_actions.has(actionId))
        throw std::invalid_argument("Action \"" + actionId + "\" is not registered");
    _items.push_back(actionId);
}

void Toolbar::addSeparator()
{
    _items.emplace_back();
}

void Toolbar::draw() const
{
    if (!_visible)
        return;

    const ImGuiStyle &style = ImGui::GetStyle();
    const float height = ImGui::GetFrameHeight() + style.WindowPadding.y * 2.0f;
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings;
    if (ImGui::BeginViewportSideBar("##Toolbar", ImGui::GetMainViewport(), ImGuiDir_Up, height, flags)) {
        bool first = true;
        for (const std::string &id : _items) {
            if (!first)
                ImGui::SameLine();
            first = false;
            if (id.empty()) {
                ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
                continue;
            }
            if (const Action *action = _actions.get(id))
                drawButton(*action);
        }
    }
    ImGui::End();
}

void Toolbar::drawButton(const Action &action) const
{
    const bool checked = action.checked();
    // Active toggles use the theme accent so they stand out from plain buttons.
    if (checked) {
        const InteractiveColors accent = ThemeManager::active().interactive(ThemeManager::active().colors.accent);
        ImGui::PushStyleColor(ImGuiCol_Button, accent.base);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, accent.hovered);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, accent.active);
        ImGui::PushStyleColor(ImGuiCol_Text, ThemeManager::active().colors.textOnAccent);
    }
    ImGui::BeginDisabled(!action.enabled());

    const std::string label = action.label + "##" + action.id;
    if (ImGui::Button(label.c_str()) && action.callback)
        action.callback();

    ImGui::EndDisabled();
    if (checked)
        ImGui::PopStyleColor(4);

    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled | ImGuiHoveredFlags_DelayShort)) {
        const std::string shortcut = _actions.shortcutLabel(action);
        ImGui::BeginTooltip();
        ImGui::TextUnformatted(action.tooltip.empty() ? action.label.c_str() : action.tooltip.c_str());
        if (!shortcut.empty())
            ImGui::TextDisabled("%s", shortcut.c_str());
        ImGui::EndTooltip();
    }
}
