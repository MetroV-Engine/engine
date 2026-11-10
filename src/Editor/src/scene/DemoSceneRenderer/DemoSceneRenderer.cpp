/*
    Created on: 30/03/2026
    Filename: DemoSceneRenderer.cpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "DemoSceneRenderer.hpp"
#include "scene/DemoSimulation/DemoSimulation.hpp"
#include "ui/ThemeManager/ThemeManager.hpp"

#include <algorithm>
#include <cfloat>
#include <optional>
#include <vector>

namespace {

using ObjectId = HierarchyModel::ObjectId;

struct Box
{
    ObjectId id;
    Vec2 center;
    Vec2 size;
    bool hasCollider;
};

std::vector<Box> collectBoxes(const DemoScene &scene)
{
    std::vector<Box> boxes;
    for (ObjectId id : scene.allObjects()) {
        const std::optional<Vec2> position = scene.getAs<Vec2>(id, PositionComponent, PositionProperty);
        if (!position)
            continue;
        const std::optional<Vec2> size = scene.getAs<Vec2>(id, "BoxCollider", "size");
        boxes.push_back({id, *position, size.value_or(Vec2{6, 6}), size.has_value()});
    }
    return boxes;
}

// Fits the world (objects + ground) in the view, keeping proportions.
ViewportView fitBoxes(const std::vector<Box> &boxes, ImVec2 min, ImVec2 max)
{
    float left = FLT_MAX, top = FLT_MAX, right = -FLT_MAX, bottom = DemoSimulation::GroundY;
    for (const Box &box : boxes) {
        left = std::min(left, box.center.x - box.size.x * 0.5f);
        right = std::max(right, box.center.x + box.size.x * 0.5f);
        top = std::min(top, box.center.y - box.size.y * 0.5f);
        bottom = std::max(bottom, box.center.y + box.size.y * 0.5f);
    }
    const float margin = 40.0f;
    left -= margin;
    right += margin;
    top -= margin;
    bottom += margin;
    const float scale = std::min((max.x - min.x) / std::max(right - left, 1.0f), (max.y - min.y) / std::max(bottom - top, 1.0f));
    const ImVec2 origin(min.x + ((max.x - min.x) - (right - left) * scale) * 0.5f - left * scale,
        min.y + ((max.y - min.y) - (bottom - top) * scale) * 0.5f - top * scale);
    return {origin, scale};
}

bool contains(ImVec2 min, ImVec2 max, ImVec2 point)
{
    return point.x >= min.x && point.x <= max.x && point.y >= min.y && point.y <= max.y;
}

} // namespace

ViewportView DemoSceneRenderer::draw(ImDrawList *drawList, ImVec2 min, ImVec2 max)
{
    const Theme &theme = ThemeManager::active();

    const std::vector<Box> boxes = collectBoxes(_scene);
    if (boxes.empty()) {
        const char *text = "The scene is empty: create an object in the Hierarchy (Ctrl+N).";
        const ImVec2 size = ImGui::CalcTextSize(text);
        drawList->AddText(ImVec2((min.x + max.x - size.x) * 0.5f, (min.y + max.y - size.y) * 0.5f),
            ImGui::GetColorU32(theme.colors.textMuted), text);
        _view = {};
        return {};
    }

    // Fitted again only when needed: fitting at every frame would move the
    // view under an object dragged with the mouse.
    const bool resized = min.x != _fittedMin.x || min.y != _fittedMin.y || max.x != _fittedMax.x || max.y != _fittedMax.y;
    if (!_view.valid() || resized || boxes.size() != _fittedObjects
        || std::ranges::any_of(boxes, [&](const Box &box) { return !contains(min, max, _view.toScreen(box.center)); })) {
        _view = fitBoxes(boxes, min, max);
        _fittedMin = min;
        _fittedMax = max;
        _fittedObjects = boxes.size();
    }
    const auto toScreen = [&](float x, float y) { return _view.toScreen(Vec2{x, y}); };

    // Ground line, across the Viewport.
    const ImU32 muted = ImGui::GetColorU32(theme.colors.textMuted);
    const float groundY = toScreen(0.0f, DemoSimulation::GroundY).y;
    drawList->AddLine(ImVec2(min.x, groundY), ImVec2(max.x, groundY), muted, 2.0f);

    for (const Box &box : boxes) {
        const ImVec2 a = toScreen(box.center.x - box.size.x * 0.5f, box.center.y - box.size.y * 0.5f);
        const ImVec2 b = toScreen(box.center.x + box.size.x * 0.5f, box.center.y + box.size.y * 0.5f);
        const bool selected = _selection.is(box.id);
        const ImVec4 &color = selected ? theme.colors.warning
            : _scene.get(box.id, "PlayerController", "movement.speed").index() != 0 ? theme.colors.success
            : _scene.get(box.id, "EnemyAI", "behavior").index() != 0                ? theme.colors.danger
                                                                                    : theme.colors.accent;
        drawList->AddRectFilled(a, b, ImGui::GetColorU32(ImVec4(color.x, color.y, color.z, box.hasCollider ? 0.35f : 0.8f)));
        drawList->AddRect(a, b, ImGui::GetColorU32(color), 0.0f, 0, selected ? 2.0f : 1.0f);
    }
    return _view;
}
