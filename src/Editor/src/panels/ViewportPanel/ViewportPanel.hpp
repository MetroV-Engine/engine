/*
    Created on: 30/03/2026
    Filename: ViewportPanel.hpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#pragma once

#include "panels/Panel/Panel.hpp"
#include "ui/PerformanceOverlay/PerformanceOverlay.hpp"
#include "scene/PlayController/PlayController.hpp"

#include <functional>

// Game view, at the center of the editor. Until the engine renders into a
// texture, a renderer callback draws the scene with ImGui shapes. A colored
// frame and a badge show the play state; the performance overlay is pinned
// to its corner.
class ViewportPanel : public Panel
{
public:
    using Renderer = std::function<void(ImDrawList *, ImVec2 min, ImVec2 max)>;

    ViewportPanel(PerformanceOverlay &overlay, const PlayController &play);

    // Draws the scene; without one, a placeholder is shown.
    void setRenderer(Renderer renderer) { _renderer = std::move(renderer); }

protected:
    void drawContent() override;

private:
    PerformanceOverlay &_overlay;
    const PlayController &_play;
    Renderer _renderer;
};
