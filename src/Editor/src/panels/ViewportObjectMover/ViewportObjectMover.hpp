/*
    Created on: 09/11/2026
    Filename: ViewportObjectMover.hpp
    Description: Selects and moves the objects of the Viewport with the mouse

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#pragma once

#include "scene/HierarchyModel.hpp"
#include "scene/PlayController/PlayController.hpp"
#include "scene/PropertyModel.hpp"
#include "scene/Selection.hpp"
#include "scene/ViewportView.hpp"

#include <imgui.h>

#include <optional>

// Clicking near an object in the Viewport selects it, dragging moves it, by
// writing its position through the PropertyModel. Only while the game is
// stopped, like the Entity Palette; a ring marks the selected object.
class ViewportObjectMover
{
public:
    using ObjectId = HierarchyModel::ObjectId;

    // Everything given must outlive the mover.
    ViewportObjectMover(const HierarchyModel &model, PropertyModel &properties, Selection &selection, const PlayController &play);

    // Call right after the item covering the Viewport.
    void update(const ViewportView &view, ImDrawList *drawList);

private:
    const HierarchyModel &_model;
    PropertyModel &_properties;
    Selection &_selection;
    const PlayController &_play;

    ObjectId _dragged = HierarchyModel::InvalidId;
    // The view when the drag started: a renderer that fits the view to its
    // objects would otherwise change it under the mouse as the object moves.
    ViewportView _dragView;
    // From the mouse to the object, so it does not jump when grabbed off-center.
    Vec2 _grabOffset;

    std::optional<Vec2> positionOf(ObjectId id) const;
    void startDrag(const ViewportView &view);
    void continueDrag();
    void drawSelection(const ViewportView &view, ImDrawList *drawList) const;
};
