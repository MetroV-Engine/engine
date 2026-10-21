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

namespace {

struct VariantColors {
    ImVec4 base;
    ImVec4 hovered;
    ImVec4 active;
};

VariantColors colorsFor(Button::Variant variant)
{
    switch (variant) {
    case Button::Variant::Secondary:
        return {{0.35f, 0.35f, 0.38f, 1.0f}, {0.45f, 0.45f, 0.48f, 1.0f}, {0.28f, 0.28f, 0.30f, 1.0f}};
    case Button::Variant::Danger:
        return {{0.75f, 0.20f, 0.20f, 1.0f}, {0.85f, 0.28f, 0.28f, 1.0f}, {0.62f, 0.15f, 0.15f, 1.0f}};
    case Button::Variant::Primary:
    default:
        return {{0.20f, 0.45f, 0.80f, 1.0f}, {0.28f, 0.55f, 0.90f, 1.0f}, {0.15f, 0.38f, 0.68f, 1.0f}};
    }
}

} // namespace

Button::Button(std::string label, OnClick onClick, Variant variant, ImVec2 size)
    : _label(std::move(label)), _onClick(std::move(onClick)), _variant(variant), _size(size)
{
}

bool Button::render() const
{
    const VariantColors colors = colorsFor(_variant);

    ImGui::PushStyleColor(ImGuiCol_Button, colors.base);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colors.hovered);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, colors.active);
    ImGui::BeginDisabled(!_enabled);

    const bool clicked = ImGui::Button(_label.c_str(), _size);

    ImGui::EndDisabled();
    ImGui::PopStyleColor(3);

    if (clicked && _onClick)
        _onClick();
    return clicked;
}
