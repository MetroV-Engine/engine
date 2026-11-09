/*
    Created on: 30/03/2026
    Filename: PerformanceOverlay.hpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#pragma once

#include "core/FrameTimer/FrameTimer.hpp"

#include <imgui.h>

// Small always-on-top overlay showing FPS, frame time and a frame time graph
// from a FrameTimer. It sits in the top-right corner of the work area, or of
// the rectangle given to setAnchor() for the current frame (e.g. the viewport).
class PerformanceOverlay
{
public:
    explicit PerformanceOverlay(const FrameTimer &timer) : _timer(timer) {}

    // Draws the overlay; forgets the anchor, which must be set again each frame.
    void render();

    // Top-right corner to stick to during this frame, in screen coordinates.
    void setAnchor(ImVec2 topRight) { _anchor = topRight; _hasAnchor = true; }

    void setVisible(bool visible) { _visible = visible; }
    bool isVisible() const { return _visible; }

private:
    const FrameTimer &_timer;
    bool _visible = true;
    ImVec2 _anchor;
    bool _hasAnchor = false;
};
