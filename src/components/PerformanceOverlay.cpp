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

#include <imgui.h>

namespace {

constexpr float Margin = 10.0f;

} // namespace

void PerformanceOverlay::render() const
{
    if (!_visible)
        return;

    // Anchored to the work area every frame, so it follows window resizes.
    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    const ImVec2 position(viewport->WorkPos.x + viewport->WorkSize.x - Margin, viewport->WorkPos.y + Margin);
    ImGui::SetNextWindowPos(position, ImGuiCond_Always, ImVec2(1.0f, 0.0f));
    ImGui::SetNextWindowBgAlpha(0.6f);

    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize
        | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav
        | ImGuiWindowFlags_NoMove;
    if (ImGui::Begin("##PerformanceOverlay", nullptr, flags)) {
        ImGui::Text("%.0f FPS", _timer.getFps());
        ImGui::Text("Frame: %.2f ms (max %.2f ms)", _timer.getAverageFrameTimeMs(), _timer.getMaxFrameTimeMs());
        const auto &history = _timer.getHistory();
        ImGui::PlotLines("##FrameTimes", history.data(), static_cast<int>(history.size()),
            static_cast<int>(_timer.getHistoryOffset()), nullptr, 0.0f, 50.0f, ImVec2(200.0f, 40.0f));
    }
    ImGui::End();
}
