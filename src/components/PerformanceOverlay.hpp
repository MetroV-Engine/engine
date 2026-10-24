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

#include "FrameTimer.hpp"

// Small always-on-top overlay in the top-right corner showing FPS, frame time
// and a frame time graph from a FrameTimer.
class PerformanceOverlay
{
public:
    explicit PerformanceOverlay(const FrameTimer &timer) : _timer(timer) {}

    void render() const;

    void setVisible(bool visible) { _visible = visible; }
    bool isVisible() const { return _visible; }

private:
    const FrameTimer &_timer;
    bool _visible = true;
};
