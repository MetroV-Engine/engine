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

#include <cstdio>
#include <optional>

ViewportPanel::ViewportPanel(PerformanceOverlay &overlay, const PlayController &play, const EntityDropTarget &entityDropTarget,
    const HierarchyModel &model, PropertyModel &properties, Selection &selection)
    : Panel("Viewport"),
      _overlay(overlay),
      _play(play),
      _entityDropTarget(entityDropTarget),
      _objectMover(model, properties, selection, play)
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
        view = _renderer(drawList, min, max);
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

    // A button rather than a Dummy: it captures the mouse for the object mover.
    ImGui::InvisibleButton("##ViewportArea", size);
    _objectMover.update(view, drawList);
    _entityDropTarget.accept(view.valid() ? std::optional<Vec2>(view.toWorld(ImGui::GetMousePos())) : std::nullopt);
    _overlay.setAnchor(ImVec2(max.x, min.y));
}
