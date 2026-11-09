/*
    Created on: 09/11/2026
    Filename: ViewportObjectMover.cpp
    Description: Selects and moves the objects of the Viewport with the mouse

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "ViewportObjectMover.hpp"
#include "ui/ThemeManager/ThemeManager.hpp"

#include <variant>
#include <vector>

namespace {

// How far from an object's position a click still grabs it.
constexpr float PickRadiusPixels = 24.0f;
constexpr float SelectionRingPixels = 14.0f;

} // namespace

ViewportObjectMover::ViewportObjectMover(
    const HierarchyModel &model, PropertyModel &properties, Selection &selection, const PlayController &play)
    : _model(model), _properties(properties), _selection(selection), _play(play)
{
}

std::optional<Vec2> ViewportObjectMover::positionOf(ObjectId id) const
{
    const PropertyValue value = _properties.get(id, PositionComponent, PositionProperty);
    const Vec2 *position = std::get_if<Vec2>(&value);
    return position ? std::optional<Vec2>(*position) : std::nullopt;
}

void ViewportObjectMover::update(const ViewportView &view, ImDrawList *drawList)
{
    if (!view.valid()) {
        _dragged = HierarchyModel::InvalidId;
        return;
    }
    if (!_play.isStopped())
        _dragged = HierarchyModel::InvalidId;
    else if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
        startDrag(view);

    if (_dragged != HierarchyModel::InvalidId)
        continueDrag();
    drawSelection(view, drawList);
}

void ViewportObjectMover::startDrag(const ViewportView &view)
{
    const ImVec2 mouse = ImGui::GetMousePos();
    float closest = PickRadiusPixels * PickRadiusPixels;
    ObjectId picked = HierarchyModel::InvalidId;
    Vec2 pickedPosition;

    std::vector<ObjectId> stack = _model.roots();
    while (!stack.empty()) {
        const ObjectId id = stack.back();
        stack.pop_back();
        for (ObjectId child : _model.children(id))
            stack.push_back(child);

        const std::optional<Vec2> position = positionOf(id);
        if (!position)
            continue;
        const ImVec2 screen = view.toScreen(*position);
        const float distance = (screen.x - mouse.x) * (screen.x - mouse.x) + (screen.y - mouse.y) * (screen.y - mouse.y);
        if (distance <= closest) {
            closest = distance;
            picked = id;
            pickedPosition = *position;
        }
    }
    if (picked == HierarchyModel::InvalidId)
        return;

    _selection.set(picked);
    _dragged = picked;
    _dragView = view;
    const Vec2 world = view.toWorld(mouse);
    _grabOffset = Vec2{pickedPosition.x - world.x, pickedPosition.y - world.y};
}

void ViewportObjectMover::continueDrag()
{
    if (!ImGui::IsItemActive() || !_model.exists(_dragged)) {
        _dragged = HierarchyModel::InvalidId;
        return;
    }
    // Past the drag threshold, so that a simple click never moves the object.
    if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
        const Vec2 world = _dragView.toWorld(ImGui::GetMousePos());
        _properties.set(_dragged, PositionComponent, PositionProperty, Vec2{world.x + _grabOffset.x, world.y + _grabOffset.y});
    }
}

void ViewportObjectMover::drawSelection(const ViewportView &view, ImDrawList *drawList) const
{
    if (!_selection.has())
        return;
    const std::optional<Vec2> position = positionOf(_selection.get());
    if (position)
        drawList->AddCircle(view.toScreen(*position), SelectionRingPixels, ImGui::GetColorU32(ThemeManager::active().colors.warning), 0, 2.0f);
}
