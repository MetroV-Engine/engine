/*
    Created on: 09/11/2026
    Filename: ViewportTransformTool.cpp
    Description: Moves and scales the objects of the Viewport with the mouse

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "ViewportTransformTool.hpp"
#include "ui/ThemeManager/ThemeManager.hpp"

#include <algorithm>
#include <cmath>

namespace {

// How far from an object's position a click still grabs it.
constexpr float PickRadiusPixels = 24.0f;
constexpr float SelectionRingPixels = 14.0f;
constexpr float HandleHalfSizePixels = 5.0f;
// How far from a handle's center a click still grabs it.
constexpr float HandlePickPixels = 9.0f;
// Each drag scales by this factor at least: a handle dragged past the
// object's position would otherwise flip or zero its scale.
constexpr float MinScaleFactor = 0.01f;

float distance(ImVec2 a, ImVec2 b)
{
    return std::hypot(a.x - b.x, a.y - b.y);
}

} // namespace

ViewportTransformTool::ViewportTransformTool(const HierarchyModel &model, PropertyModel &properties, Selection &selection,
    const PlayController &play, const ViewportGrid &grid)
    : _model(model), _properties(properties), _selection(selection), _play(play), _grid(grid)
{
}

void ViewportTransformTool::update(const ViewportView &view, ImDrawList *drawList)
{
    if (!view.valid()) {
        _dragged = HierarchyModel::InvalidId;
        return;
    }
    const bool stopped = _play.isStopped();
    if (!stopped)
        _dragged = HierarchyModel::InvalidId;
    else if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
        startDrag(view);

    if (_dragged != HierarchyModel::InvalidId)
        continueDrag();
    drawSelection(view, drawList);
    if (stopped)
        drawHandles(view, drawList);
}

std::optional<ImVec2> ViewportTransformTool::handlesCenter(const ViewportView &view) const
{
    if (!_selection.has() || !scaleOf(_selection.get()))
        return std::nullopt;
    const std::optional<Vec2> position = positionOf(_selection.get());
    return position ? std::optional<ImVec2>(view.toScreen(*position)) : std::nullopt;
}

ImVec2 ViewportTransformTool::handlePosition(ImVec2 center, Handle handle)
{
    switch (handle) {
    case Handle::ScaleX: return ImVec2(center.x + HandleDistancePixels, center.y);
    case Handle::ScaleY: return ImVec2(center.x, center.y - HandleDistancePixels);
    case Handle::ScaleBoth: return ImVec2(center.x + HandleDistancePixels, center.y - HandleDistancePixels);
    case Handle::Position: break;
    }
    return center;
}

std::optional<ViewportTransformTool::Handle> ViewportTransformTool::handleAt(ImVec2 center, ImVec2 mouse)
{
    for (const Handle handle : {Handle::ScaleX, Handle::ScaleY, Handle::ScaleBoth})
        if (distance(mouse, handlePosition(center, handle)) <= HandlePickPixels)
            return handle;
    return std::nullopt;
}

void ViewportTransformTool::startDrag(const ViewportView &view)
{
    const ImVec2 mouse = ImGui::GetMousePos();
    _dragView = view;

    // The handles, drawn over the objects, come first.
    if (const std::optional<ImVec2> center = handlesCenter(view))
        if (const std::optional<Handle> handle = handleAt(*center, mouse)) {
            _dragged = _selection.get();
            _handle = *handle;
            _startScale = *scaleOf(_dragged);
            _center = *center;
            _grab = ImVec2(mouse.x - center->x, mouse.y - center->y);
            return;
        }

    float closestSquared = PickRadiusPixels * PickRadiusPixels;
    ObjectId picked = HierarchyModel::InvalidId;
    Vec2 pickedPosition;
    for (ObjectId id : _model.allObjects()) {
        const std::optional<Vec2> position = positionOf(id);
        if (!position)
            continue;
        const ImVec2 screen = view.toScreen(*position);
        const float distanceSquared = (screen.x - mouse.x) * (screen.x - mouse.x) + (screen.y - mouse.y) * (screen.y - mouse.y);
        if (distanceSquared <= closestSquared) {
            closestSquared = distanceSquared;
            picked = id;
            pickedPosition = *position;
        }
    }
    if (picked == HierarchyModel::InvalidId)
        return;

    _selection.set(picked);
    _dragged = picked;
    _handle = Handle::Position;
    const Vec2 world = view.toWorld(mouse);
    _grabOffset = Vec2{pickedPosition.x - world.x, pickedPosition.y - world.y};
}

void ViewportTransformTool::continueDrag()
{
    if (!ImGui::IsItemActive() || !_model.exists(_dragged)) {
        _dragged = HierarchyModel::InvalidId;
        return;
    }
    // Past the drag threshold, so that a simple click never changes the object.
    if (!ImGui::IsMouseDragging(ImGuiMouseButton_Left))
        return;
    const ImVec2 mouse = ImGui::GetMousePos();
    if (_handle == Handle::Position) {
        const Vec2 world = _dragView.toWorld(mouse);
        const Vec2 position{world.x + _grabOffset.x, world.y + _grabOffset.y};
        _properties.set(_dragged, TransformComponent, PositionProperty, _grid.snapPosition(position));
    } else {
        _properties.set(_dragged, TransformComponent, ScaleProperty, _grid.snapScale(draggedScale(mouse)));
    }
}

Vec2 ViewportTransformTool::draggedScale(ImVec2 mouse) const
{
    // The grabbed point follows the mouse: the scale grows with its distance
    // from the object.
    const ImVec2 from(mouse.x - _center.x, mouse.y - _center.y);
    const auto factor = [](float now, float start) { return std::max(now / start, MinScaleFactor); };
    Vec2 scale = _startScale;
    switch (_handle) {
    case Handle::ScaleX: scale.x *= factor(from.x, _grab.x); break;
    case Handle::ScaleY: scale.y *= factor(from.y, _grab.y); break;
    case Handle::ScaleBoth: {
        const float both = factor(std::hypot(from.x, from.y), std::hypot(_grab.x, _grab.y));
        scale.x *= both;
        scale.y *= both;
        break;
    }
    case Handle::Position: break;
    }
    return scale;
}

void ViewportTransformTool::drawSelection(const ViewportView &view, ImDrawList *drawList) const
{
    if (!_selection.has())
        return;
    const std::optional<Vec2> position = positionOf(_selection.get());
    if (position)
        drawList->AddCircle(view.toScreen(*position), SelectionRingPixels, ImGui::GetColorU32(ThemeManager::active().colors.warning), 0, 2.0f);
}

void ViewportTransformTool::drawHandles(const ViewportView &view, ImDrawList *drawList) const
{
    const std::optional<ImVec2> center = handlesCenter(view);
    if (!center)
        return;
    const bool scaling = _dragged != HierarchyModel::InvalidId && _handle != Handle::Position;
    const std::optional<Handle> active =
        scaling ? std::optional<Handle>(_handle) : ImGui::IsItemHovered() ? handleAt(*center, ImGui::GetMousePos()) : std::nullopt;

    const auto &colors = ThemeManager::active().colors;
    for (const Handle handle : {Handle::ScaleX, Handle::ScaleY, Handle::ScaleBoth}) {
        const ImVec2 at = handlePosition(*center, handle);
        const ImU32 color =
            ImGui::GetColorU32(handle == Handle::ScaleX ? colors.danger : handle == Handle::ScaleY ? colors.success : colors.accent);
        if (handle != Handle::ScaleBoth)
            drawList->AddLine(*center, at, color, 2.0f);
        const float half = handle == active ? HandleHalfSizePixels + 2.0f : HandleHalfSizePixels;
        drawList->AddRectFilled(ImVec2(at.x - half, at.y - half), ImVec2(at.x + half, at.y + half), color);
    }
    if (active)
        ImGui::SetMouseCursor(*active == Handle::ScaleX   ? ImGuiMouseCursor_ResizeEW
                              : *active == Handle::ScaleY ? ImGuiMouseCursor_ResizeNS
                                                          : ImGuiMouseCursor_ResizeNESW);
}
