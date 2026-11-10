/*
    Created on: 09/11/2026
    Filename: ViewportTransformTool.hpp
    Description: Moves and scales the objects of the Viewport with the mouse

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
#include "scene/ViewportGrid.hpp"
#include "scene/ViewportView.hpp"

#include <imgui.h>

#include <optional>

// Edits the Transform of the Viewport's objects with the mouse, through the
// PropertyModel, only while the game is stopped (like the Entity Palette).
// Clicking near an object selects it and dragging moves it. The selected
// object, marked by a ring, also gets scale handles if it has a scale: one
// per axis and one for both. Positions and scales snap to the grid when it
// is on.
class ViewportTransformTool
{
public:
    using ObjectId = HierarchyModel::ObjectId;

    // On screen, from an object's position to its scale handles.
    static constexpr float HandleDistancePixels = 48.0f;

    // Everything given must outlive the tool.
    ViewportTransformTool(const HierarchyModel &model, PropertyModel &properties, Selection &selection, const PlayController &play,
        const ViewportGrid &grid);

    // Call right after the item covering the Viewport.
    void update(const ViewportView &view, ImDrawList *drawList);

private:
    // What a drag changes.
    enum class Handle
    {
        Position,
        ScaleX,
        ScaleY,
        ScaleBoth,
    };

    const HierarchyModel &_model;
    PropertyModel &_properties;
    Selection &_selection;
    const PlayController &_play;
    const ViewportGrid &_grid;

    ObjectId _dragged = HierarchyModel::InvalidId;
    Handle _handle = Handle::Position;
    // The view when the drag started: a renderer that fits the view to its
    // objects would otherwise change it under the mouse as the object changes.
    ViewportView _dragView;
    // Moving: from the mouse to the object, so it does not jump when grabbed off-center.
    Vec2 _grabOffset;
    // Scaling: the scale when the drag started, the object on screen, and
    // where the mouse grabbed the handle, from the object.
    Vec2 _startScale;
    ImVec2 _center;
    ImVec2 _grab;

    std::optional<Vec2> positionOf(ObjectId id) const { return _properties.getAs<Vec2>(id, TransformComponent, PositionProperty); }
    std::optional<Vec2> scaleOf(ObjectId id) const { return _properties.getAs<Vec2>(id, TransformComponent, ScaleProperty); }
    // The selected object on screen, if it has a position and a scale.
    std::optional<ImVec2> handlesCenter(const ViewportView &view) const;
    static ImVec2 handlePosition(ImVec2 center, Handle handle);
    static std::optional<Handle> handleAt(ImVec2 center, ImVec2 mouse);

    void startDrag(const ViewportView &view);
    void continueDrag();
    Vec2 draggedScale(ImVec2 mouse) const;
    void drawSelection(const ViewportView &view, ImDrawList *drawList) const;
    void drawHandles(const ViewportView &view, ImDrawList *drawList) const;
};
