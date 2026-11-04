/*
    Created on: 30/03/2026
    Filename: Theme.hpp
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

#include <string>

// Semantic colors: components ask for "accent" or "danger", never for a raw
// RGB value, so changing the theme restyles the whole editor.
struct ThemeColors
{
    ImVec4 background;    // Behind every window (renderer clear color)
    ImVec4 surface;       // Window background
    ImVec4 surfaceRaised; // Popups, menus, title bars, tooltips
    ImVec4 surfaceSunken; // Input fields, checkboxes, sliders
    ImVec4 border;
    ImVec4 text;
    ImVec4 textMuted;     // Secondary or disabled text
    ImVec4 textOnAccent;  // Text on accent/danger/success backgrounds
    ImVec4 accent;        // Primary actions, selection, focus
    ImVec4 secondary;     // Neutral buttons, headers
    ImVec4 danger;        // Destructive actions, errors
    ImVec4 success;
    ImVec4 warning;
};

// Shape and spacing, in unscaled pixels (the display scale is applied on top).
struct ThemeMetrics
{
    float windowRounding = 6.0f;
    float frameRounding = 4.0f;
    float popupRounding = 4.0f;
    float grabRounding = 4.0f;
    float tabRounding = 4.0f;
    float scrollbarRounding = 6.0f;
    float windowBorderSize = 1.0f;
    float frameBorderSize = 0.0f;
    float popupBorderSize = 1.0f;
    float scrollbarSize = 12.0f;
    ImVec2 windowPadding = ImVec2(10.0f, 10.0f);
    ImVec2 framePadding = ImVec2(8.0f, 4.0f);
    ImVec2 itemSpacing = ImVec2(8.0f, 6.0f);
    ImVec2 itemInnerSpacing = ImVec2(6.0f, 4.0f);
    float fontScale = 1.0f;
    // Background opacity of floating overlays (e.g. the performance overlay).
    float overlayAlpha = 0.6f;
};

// Normal, hovered and pressed shades of an interactive color.
struct InteractiveColors
{
    ImVec4 base;
    ImVec4 hovered;
    ImVec4 active;
};

struct Theme
{
    std::string name;
    bool dark = true;
    ThemeColors colors;
    ThemeMetrics metrics;

    // Writes the whole theme (colors + metrics) into an ImGui style.
    void applyTo(ImGuiStyle &style) const;

    // Hover/pressed shades derived from a base color: lighter on a dark
    // theme, darker on a light one.
    InteractiveColors interactive(const ImVec4 &base) const;

    // Built-in themes.
    static Theme makeDark();
    static Theme makeLight();
};
