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
#include "scene/ViewportView.hpp"

#include <functional>

// Game view, at the center of the editor. Until the engine renders into a
// texture, a renderer callback draws the scene with ImGui shapes. A colored
// frame and a badge show the play state; the performance overlay is pinned
// to its corner. Entity templates can be dropped on it, and its objects moved
// with the mouse.
class EntityDropTarget;
class ViewportObjectMover;

class ViewportPanel : public Panel
{
public:
    using Renderer = std::function<void(ImDrawList *, ImVec2 min, ImVec2 max)>;

    ViewportPanel(PerformanceOverlay &overlay, const PlayController &play);

    // Draws the scene; without one, a placeholder is shown.
    void setRenderer(Renderer renderer) { _renderer = std::move(renderer); }
    // How the renderer maps the world to this panel.
    void setViewProvider(ViewportViewProvider provider) { _viewProvider = std::move(provider); }
    // Where dropped entity templates go; target must outlive the panel.
    void setEntityDropTarget(const EntityDropTarget *target) { _entityDropTarget = target; }
    // Selects and moves objects with the mouse; mover must outlive the panel.
    void setObjectMover(ViewportObjectMover *mover) { _objectMover = mover; }

protected:
    void drawContent() override;

private:
    PerformanceOverlay &_overlay;
    const PlayController &_play;
    Renderer _renderer;
    ViewportViewProvider _viewProvider;
    const EntityDropTarget *_entityDropTarget = nullptr;
    ViewportObjectMover *_objectMover = nullptr;
};
