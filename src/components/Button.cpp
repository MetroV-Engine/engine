/*
    Created on: 30/03/2026
    Filename: Button.cpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "Button.hpp"
#include "theme/ThemeManager.hpp"

Button::Button(std::string label, OnClick onClick, Variant variant, ImVec2 size)
    : _label(std::move(label)), _onClick(std::move(onClick)), _variant(variant), _size(size)
{
}

bool Button::render() const
{
    const Theme &theme = ThemeManager::active();
    const ImVec4 &base = _variant == Variant::Danger  ? theme.colors.danger
                         : _variant == Variant::Secondary ? theme.colors.secondary
                                                          : theme.colors.accent;
    const InteractiveColors colors = theme.interactive(base);

    ImGui::PushStyleColor(ImGuiCol_Button, colors.base);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colors.hovered);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, colors.active);
    // Secondary buttons sit on the surface and keep the normal text color.
    ImGui::PushStyleColor(ImGuiCol_Text, _variant == Variant::Secondary ? theme.colors.text : theme.colors.textOnAccent);
    ImGui::BeginDisabled(!_enabled);

    const bool clicked = ImGui::Button(_label.c_str(), _size);

    ImGui::EndDisabled();
    ImGui::PopStyleColor(4);

    if (clicked && _onClick)
        _onClick();
    return clicked;
}
