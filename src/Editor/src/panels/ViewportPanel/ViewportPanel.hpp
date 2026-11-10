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
#include "panels/ViewportTransformTool/ViewportTransformTool.hpp"
#include "ui/PerformanceOverlay/PerformanceOverlay.hpp"
#include "scene/PlayController/PlayController.hpp"
#include "scene/ViewportGrid.hpp"
#include "scene/ViewportView.hpp"

#include <imgui.h>

class EntityDropTarget;

// Game view, at the center of the editor. Until the engine renders into a
// texture, a renderer callback draws the scene with ImGui shapes. A colored
// frame and a badge show the play state; the performance overlay is pinned
// to its corner. Entity templates can be dropped on it, and its objects moved
// and scaled with the mouse; the grid they snap to is drawn under them.
class ViewportPanel : public Panel
{
public:
    using Renderer = ViewportRenderer;

    // Everything given must outlive the panel.
    ViewportPanel(PerformanceOverlay &overlay, const PlayController &play, const EntityDropTarget &entityDropTarget,
        const HierarchyModel &model, PropertyModel &properties, Selection &selection, const ViewportGrid &grid);

    // Draws the scene; without one, a placeholder is shown.
    void setRenderer(Renderer renderer) { _renderer = std::move(renderer); }

protected:
    void drawContent() override;

private:
    PerformanceOverlay &_overlay;
    const PlayController &_play;
    const EntityDropTarget &_entityDropTarget;
    const ViewportGrid &_grid;
    ViewportTransformTool _transformTool;
    Renderer _renderer;
    // The grid under the game, drawn after it: it needs the view the renderer returns.
    ImDrawListSplitter _layers;
};
