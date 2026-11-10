/*
    Created on: 30/03/2026
    Filename: ViewportPanel.cpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "ViewportPanel.hpp"
#include "panels/EntityDropTarget/EntityDropTarget.hpp"
#include "ui/ThemeManager/ThemeManager.hpp"

#include <cmath>
#include <cstdio>
#include <optional>

namespace {

// Closer grid lines would hide the game: none are drawn then.
constexpr float MinGridSpacingPixels = 6.0f;

void drawGrid(ImDrawList *drawList, const ViewportView &view, const ViewportGrid &grid, ImVec2 min, ImVec2 max)
{
    const float step = static_cast<float>(grid.size);
    if (!grid.snap || !view.valid() || step * view.scale < MinGridSpacingPixels)
        return;
    const ImU32 color = ImGui::GetColorU32(ThemeManager::active().colors.border);
    const Vec2 first = view.toWorld(min);
    const Vec2 last = view.toWorld(max);
    for (float x = std::ceil(first.x / step) * step; x <= last.x; x += step) {
        const float screenX = view.toScreen(Vec2{x, 0.0f}).x;
        drawList->AddLine(ImVec2(screenX, min.y), ImVec2(screenX, max.y), color);
    }
    for (float y = std::ceil(first.y / step) * step; y <= last.y; y += step) {
        const float screenY = view.toScreen(Vec2{0.0f, y}).y;
        drawList->AddLine(ImVec2(min.x, screenY), ImVec2(max.x, screenY), color);
    }
}

} // namespace

ViewportPanel::ViewportPanel(PerformanceOverlay &overlay, const PlayController &play, const EntityDropTarget &entityDropTarget,
    const HierarchyModel &model, PropertyModel &properties, Selection &selection, const ViewportGrid &grid)
    : Panel("Viewport"),
      _overlay(overlay),
      _play(play),
      _entityDropTarget(entityDropTarget),
      _grid(grid),
      _transformTool(model, properties, selection, play, grid)
{
    // The game image goes edge to edge.
    setWindowPadding(ImVec2(0.0f, 0.0f));
}

void ViewportPanel::drawContent()
{
    const Theme &theme = ThemeManager::active();
    const ImVec2 min = ImGui::GetCursorScreenPos();
    const ImVec2 size = ImGui::GetContentRegionAvail();
    if (size.x <= 0.0f || size.y <= 0.0f)
        return;
    const ImVec2 max(min.x + size.x, min.y + size.y);
    ImDrawList *drawList = ImGui::GetWindowDrawList();
    drawList->AddRectFilled(min, max, ImGui::GetColorU32(theme.colors.background));

    // Will become ImGui::Image(gameTexture, size) once the engine renders.
    ViewportView view;
    if (_renderer) {
        drawList->PushClipRect(min, max, true);
        _layers.Split(drawList, 2);
        _layers.SetCurrentChannel(drawList, 1);
        view = _renderer(drawList, min, max);
        _layers.SetCurrentChannel(drawList, 0);
        drawGrid(drawList, view, _grid, min, max);
        _layers.Merge(drawList);
        drawList->PopClipRect();
    } else {
        const char *title = "Game viewport";
        const char *subtitle = "Engine renderer not connected yet";
        const ImVec2 titleSize = ImGui::CalcTextSize(title);
        const ImVec2 subtitleSize = ImGui::CalcTextSize(subtitle);
        const float centerX = min.x + size.x * 0.5f;
        const float centerY = min.y + size.y * 0.5f;
        drawList->AddText(ImVec2(centerX - titleSize.x * 0.5f, centerY - titleSize.y), ImGui::GetColorU32(theme.colors.text), title);
        drawList->AddText(ImVec2(centerX - subtitleSize.x * 0.5f, centerY + 2.0f), ImGui::GetColorU32(theme.colors.textMuted), subtitle);
    }

    // Play mode is impossible to miss: colored frame and a badge.
    if (!_play.isStopped()) {
        const bool playing = _play.getState() == PlayState::Playing;
        const ImU32 color = ImGui::GetColorU32(playing ? theme.colors.success : theme.colors.warning);
        drawList->AddRect(ImVec2(min.x + 1, min.y + 1), ImVec2(max.x - 1, max.y - 1), color, 0.0f, 0, 3.0f);
        char badge[64];
        std::snprintf(badge, sizeof(badge), "%s  %.1fs", PlayController::stateName(_play.getState()), _play.getPlayTime());
        const ImVec2 padding = ImGui::GetStyle().FramePadding;
        const ImVec2 textSize = ImGui::CalcTextSize(badge);
        const ImVec2 badgeMin(min.x + padding.x * 2, min.y + padding.y * 2);
        const ImVec2 badgeMax(badgeMin.x + textSize.x + padding.x * 2, badgeMin.y + textSize.y + padding.y * 2);
        drawList->AddRectFilled(badgeMin, badgeMax, color, ImGui::GetStyle().FrameRounding);
        drawList->AddText(ImVec2(badgeMin.x + padding.x, badgeMin.y + padding.y), ImGui::GetColorU32(theme.colors.textOnAccent), badge);
    }

    // A button rather than a Dummy: it captures the mouse for the transform tool.
    ImGui::InvisibleButton("##ViewportArea", size);
    _transformTool.update(view, drawList);
    _entityDropTarget.accept(view.valid() ? std::optional<Vec2>(_grid.snapPosition(view.toWorld(ImGui::GetMousePos()))) : std::nullopt);
    _overlay.setAnchor(ImVec2(max.x, min.y));
}
