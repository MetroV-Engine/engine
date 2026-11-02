/*
    Created on: 30/03/2026
    Filename: Theme.cpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "Theme.hpp"

#include <cmath>

namespace {

ImVec4 rgb(int r, int g, int b, float a = 1.0f)
{
    return ImVec4(r / 255.0f, g / 255.0f, b / 255.0f, a);
}

// Moves a color towards white (amount > 0) or black (amount < 0).
ImVec4 shade(const ImVec4 &c, float amount)
{
    const float target = amount > 0.0f ? 1.0f : 0.0f;
    const float t = std::abs(amount);
    return ImVec4(c.x + (target - c.x) * t, c.y + (target - c.y) * t, c.z + (target - c.z) * t, c.w);
}

ImVec4 withAlpha(const ImVec4 &c, float alpha)
{
    return ImVec4(c.x, c.y, c.z, alpha);
}

} // namespace

InteractiveColors Theme::interactive(const ImVec4 &base) const
{
    const float direction = dark ? 1.0f : -1.0f;
    return {base, shade(base, 0.12f * direction), shade(base, -0.12f * direction)};
}

void Theme::applyTo(ImGuiStyle &style) const
{
    // Start from ImGui's own preset so colors not listed below stay coherent.
    style = ImGuiStyle();
    if (dark)
        ImGui::StyleColorsDark(&style);
    else
        ImGui::StyleColorsLight(&style);

    style.WindowRounding = metrics.windowRounding;
    style.ChildRounding = metrics.windowRounding;
    style.FrameRounding = metrics.frameRounding;
    style.PopupRounding = metrics.popupRounding;
    style.GrabRounding = metrics.grabRounding;
    style.TabRounding = metrics.tabRounding;
    style.ScrollbarRounding = metrics.scrollbarRounding;
    style.WindowBorderSize = metrics.windowBorderSize;
    style.FrameBorderSize = metrics.frameBorderSize;
    style.PopupBorderSize = metrics.popupBorderSize;
    style.ScrollbarSize = metrics.scrollbarSize;
    style.WindowPadding = metrics.windowPadding;
    style.FramePadding = metrics.framePadding;
    style.ItemSpacing = metrics.itemSpacing;
    style.ItemInnerSpacing = metrics.itemInnerSpacing;
    style.FontScaleMain = metrics.fontScale;

    const ThemeColors &c = colors;
    const InteractiveColors accent = interactive(c.accent);
    const InteractiveColors secondary = interactive(c.secondary);
    const InteractiveColors sunken = interactive(c.surfaceSunken);
    ImVec4 *col = style.Colors;

    col[ImGuiCol_Text] = c.text;
    col[ImGuiCol_TextDisabled] = c.textMuted;
    col[ImGuiCol_WindowBg] = c.surface;
    col[ImGuiCol_ChildBg] = withAlpha(c.surface, 0.0f);
    col[ImGuiCol_PopupBg] = c.surfaceRaised;
    col[ImGuiCol_Border] = c.border;
    col[ImGuiCol_BorderShadow] = withAlpha(c.border, 0.0f);

    col[ImGuiCol_FrameBg] = sunken.base;
    col[ImGuiCol_FrameBgHovered] = sunken.hovered;
    col[ImGuiCol_FrameBgActive] = sunken.active;

    col[ImGuiCol_TitleBg] = c.surfaceRaised;
    col[ImGuiCol_TitleBgActive] = shade(c.surfaceRaised, dark ? 0.06f : -0.06f);
    col[ImGuiCol_TitleBgCollapsed] = withAlpha(c.surfaceRaised, 0.75f);
    col[ImGuiCol_MenuBarBg] = c.surfaceRaised;

    col[ImGuiCol_ScrollbarBg] = withAlpha(c.surface, 0.0f);
    col[ImGuiCol_ScrollbarGrab] = secondary.base;
    col[ImGuiCol_ScrollbarGrabHovered] = secondary.hovered;
    col[ImGuiCol_ScrollbarGrabActive] = secondary.active;

    col[ImGuiCol_CheckMark] = accent.base;
    col[ImGuiCol_SliderGrab] = accent.base;
    col[ImGuiCol_SliderGrabActive] = accent.active;

    col[ImGuiCol_Button] = secondary.base;
    col[ImGuiCol_ButtonHovered] = secondary.hovered;
    col[ImGuiCol_ButtonActive] = secondary.active;

    col[ImGuiCol_Header] = withAlpha(accent.base, 0.45f);
    col[ImGuiCol_HeaderHovered] = withAlpha(accent.hovered, 0.65f);
    col[ImGuiCol_HeaderActive] = accent.active;

    col[ImGuiCol_Separator] = c.border;
    col[ImGuiCol_SeparatorHovered] = accent.hovered;
    col[ImGuiCol_SeparatorActive] = accent.active;
    col[ImGuiCol_ResizeGrip] = withAlpha(accent.base, 0.2f);
    col[ImGuiCol_ResizeGripHovered] = withAlpha(accent.hovered, 0.6f);
    col[ImGuiCol_ResizeGripActive] = accent.active;

    col[ImGuiCol_Tab] = c.surfaceRaised;
    col[ImGuiCol_TabHovered] = accent.hovered;
    col[ImGuiCol_TabSelected] = accent.base;
    col[ImGuiCol_TabSelectedOverline] = accent.active;
    col[ImGuiCol_TabDimmed] = c.surfaceRaised;
    col[ImGuiCol_TabDimmedSelected] = secondary.base;

    col[ImGuiCol_PlotLines] = accent.base;
    col[ImGuiCol_PlotLinesHovered] = c.warning;
    col[ImGuiCol_PlotHistogram] = accent.base;
    col[ImGuiCol_PlotHistogramHovered] = c.warning;

    col[ImGuiCol_TextSelectedBg] = withAlpha(accent.base, 0.35f);
    col[ImGuiCol_TextLink] = accent.hovered;
    col[ImGuiCol_InputTextCursor] = c.text;
    col[ImGuiCol_DragDropTarget] = c.warning;
    col[ImGuiCol_NavCursor] = accent.hovered;
    // Darkens everything behind a modal dialog, so it stands out.
    col[ImGuiCol_ModalWindowDimBg] = dark ? ImVec4(0.0f, 0.0f, 0.0f, 0.55f) : ImVec4(0.1f, 0.1f, 0.15f, 0.35f);
}

Theme Theme::makeDark()
{
    Theme theme;
    theme.name = "Dark";
    theme.dark = true;
    theme.colors = {
        .background = rgb(24, 25, 28),
        .surface = rgb(32, 33, 37),
        .surfaceRaised = rgb(40, 42, 47),
        .surfaceSunken = rgb(22, 23, 26),
        .border = rgb(55, 57, 64),
        .text = rgb(224, 226, 232),
        .textMuted = rgb(130, 134, 145),
        .textOnAccent = rgb(255, 255, 255),
        .accent = rgb(58, 120, 214),
        .secondary = rgb(68, 71, 80),
        .danger = rgb(200, 62, 62),
        .success = rgb(70, 170, 100),
        .warning = rgb(230, 170, 60),
    };
    return theme;
}

Theme Theme::makeLight()
{
    Theme theme;
    theme.name = "Light";
    theme.dark = false;
    theme.colors = {
        .background = rgb(222, 224, 229),
        .surface = rgb(246, 247, 249),
        .surfaceRaised = rgb(232, 234, 238),
        .surfaceSunken = rgb(255, 255, 255),
        .border = rgb(196, 199, 207),
        .text = rgb(30, 32, 38),
        .textMuted = rgb(120, 124, 135),
        .textOnAccent = rgb(255, 255, 255),
        .accent = rgb(40, 105, 205),
        .secondary = rgb(208, 211, 218),
        .danger = rgb(200, 50, 50),
        .success = rgb(40, 145, 80),
        .warning = rgb(200, 135, 20),
    };
    theme.metrics.frameBorderSize = 1.0f;
    theme.metrics.overlayAlpha = 0.85f;
    return theme;
}
