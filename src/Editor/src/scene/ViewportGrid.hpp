/*
    Created on: 10/11/2026
    Filename: ViewportGrid.hpp
    Description: Snapping of the positions and scales edited in the Viewport

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#pragma once

#include "scene/PropertyModel.hpp"

#include <cmath>

// Snapping of what the mouse edits in the Viewport, set in the Settings
// panel: positions to a grid of world units, scales to tenths. Off by
// default; the Viewport draws the grid while it is on.
struct ViewportGrid
{
    static constexpr int DefaultSize = 16;
    static constexpr int MinSize = 1;
    static constexpr int MaxSize = 1024;
    static constexpr float ScaleStep = 0.1f;

    bool snap = false;
    // World units between two grid lines.
    int size = DefaultSize;

    Vec2 snapPosition(Vec2 position) const
    {
        if (!snap)
            return position;
        const float step = static_cast<float>(size);
        return Vec2{std::round(position.x / step) * step, std::round(position.y / step) * step};
    }

    // Never to 0, which would make the object vanish.
    Vec2 snapScale(Vec2 scale) const
    {
        if (!snap)
            return scale;
        const auto snapped = [](float value) {
            const float rounded = std::round(value / ScaleStep) * ScaleStep;
            return rounded != 0.0f ? rounded : std::copysign(ScaleStep, value);
        };
        return Vec2{snapped(scale.x), snapped(scale.y)};
    }
};
