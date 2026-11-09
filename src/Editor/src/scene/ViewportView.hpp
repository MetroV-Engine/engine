/*
    Created on: 09/11/2026
    Filename: ViewportView.hpp
    Description: Conversion between the pixels of the Viewport and world positions

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#pragma once

#include "scene/PropertyModel.hpp"

#include <imgui.h>

#include <functional>

// How a renderer maps the world to the screen. Without a valid view (scale 0,
// e.g. no renderer connected) the mouse cannot place objects in the Viewport.
struct ViewportView
{
    // Screen position of the world's (0, 0).
    ImVec2 origin{0.0f, 0.0f};
    // Pixels per world unit.
    float scale = 0.0f;

    bool valid() const { return scale > 0.0f; }
    ImVec2 toScreen(Vec2 world) const { return ImVec2(origin.x + world.x * scale, origin.y + world.y * scale); }
    Vec2 toWorld(ImVec2 screen) const { return Vec2{(screen.x - origin.x) / scale, (screen.y - origin.y) / scale}; }
};

// Draws the game in the Viewport covering the screen rectangle min..max, and
// returns how it mapped the world there (an invalid view if it does not say).
using ViewportRenderer = std::function<ViewportView(ImDrawList *, ImVec2 min, ImVec2 max)>;

// The property placed by the mouse: objects without it keep no position.
inline constexpr const char *PositionComponent = "Transform";
inline constexpr const char *PositionProperty = "position";
