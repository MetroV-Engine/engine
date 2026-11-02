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
#include "theme/ThemeManager.hpp"

ViewportPanel::ViewportPanel(PerformanceOverlay &overlay) : Panel("Viewport"), _overlay(overlay)
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

    // Will become ImGui::Image(gameTexture, size) once the engine renders.
    ImDrawList *drawList = ImGui::GetWindowDrawList();
    drawList->AddRectFilled(min, max, ImGui::GetColorU32(theme.colors.background));
    const char *title = "Game viewport";
    const char *subtitle = "Engine renderer not connected yet";
    const ImVec2 titleSize = ImGui::CalcTextSize(title);
    const ImVec2 subtitleSize = ImGui::CalcTextSize(subtitle);
    const float centerX = min.x + size.x * 0.5f;
    const float centerY = min.y + size.y * 0.5f;
    drawList->AddText(ImVec2(centerX - titleSize.x * 0.5f, centerY - titleSize.y), ImGui::GetColorU32(theme.colors.text), title);
    drawList->AddText(ImVec2(centerX - subtitleSize.x * 0.5f, centerY + 2.0f), ImGui::GetColorU32(theme.colors.textMuted), subtitle);
    ImGui::Dummy(size);

    _overlay.setAnchor(ImVec2(max.x, min.y));
}
