/*
    Created on: 30/03/2026
    Filename: PerformanceOverlay.cpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "PerformanceOverlay.hpp"
#include "theme/ThemeManager.hpp"

#include <imgui.h>

void PerformanceOverlay::render()
{
    const bool hasAnchor = _hasAnchor;
    _hasAnchor = false;
    if (!_visible)
        return;

    const Theme &theme = ThemeManager::active();
    const float margin = ImGui::GetStyle().WindowPadding.x;

    // Anchored every frame, so it follows window and panel resizes.
    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    const ImVec2 corner = hasAnchor ? _anchor : ImVec2(viewport->WorkPos.x + viewport->WorkSize.x, viewport->WorkPos.y);
    const ImVec2 position(corner.x - margin, corner.y + margin);
    ImGui::SetNextWindowPos(position, ImGuiCond_Always, ImVec2(1.0f, 0.0f));
    ImGui::SetNextWindowBgAlpha(theme.metrics.overlayAlpha);

    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize
        | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav
        | ImGuiWindowFlags_NoMove;
    if (ImGui::Begin("##PerformanceOverlay", nullptr, flags)) {
        // Green at 60 FPS and above, orange from 30, red below; muted until
        // the first measurement is available.
        const double fps = _timer.getFps();
        if (fps <= 0.0) {
            ImGui::TextColored(theme.colors.textMuted, "-- FPS");
        } else {
            const ImVec4 &fpsColor = fps >= 60.0 ? theme.colors.success
                                     : fps >= 30.0 ? theme.colors.warning
                                                   : theme.colors.danger;
            ImGui::TextColored(fpsColor, "%.0f FPS", fps);
        }
        ImGui::Text("Frame: %.2f ms (max %.2f ms)", _timer.getAverageFrameTimeMs(), _timer.getMaxFrameTimeMs());
        const auto &history = _timer.getHistory();
        ImGui::PlotLines("##FrameTimes", history.data(), static_cast<int>(history.size()),
            static_cast<int>(_timer.getHistoryOffset()), nullptr, 0.0f, 50.0f, ImVec2(200.0f, 40.0f));
    }
    ImGui::End();
}
