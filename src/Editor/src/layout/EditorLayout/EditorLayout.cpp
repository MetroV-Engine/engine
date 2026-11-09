/*
    Created on: 30/03/2026
    Filename: EditorLayout.cpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "EditorLayout.hpp"

// The DockBuilder API (to create a default layout from code) is only in
// imgui_internal.h; it is used in this file only.
#include <imgui_internal.h>

#include <algorithm>

namespace {

constexpr const char *DockspaceName = "EditorDockspace";
// Fraction of the left column given to LeftBottom.
constexpr float LeftBottomRatio = 0.35f;

} // namespace

EditorLayout::EditorLayout()
{
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
}

void EditorLayout::setDefaultArea(const std::string &panelName, Area area)
{
    for (auto &[name, current] : _defaultAreas) {
        if (name == panelName) {
            current = area;
            return;
        }
    }
    _defaultAreas.emplace_back(panelName, area);
}

void EditorLayout::setAreaRatios(float left, float right, float bottom)
{
    _leftRatio = left;
    _rightRatio = right;
    _bottomRatio = bottom;
}

void EditorLayout::draw()
{
    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    const ImGuiID dockspaceId = ImGui::GetID(DockspaceName);

    // No saved layout (first launch, imgui.ini deleted) or reset requested.
    if (_resetRequested || !ImGui::DockBuilderGetNode(dockspaceId)) {
        buildDefaultLayout(dockspaceId, viewport->WorkSize);
        _resetRequested = false;
    }
    if (_selectFirstTabsIn > 0 && --_selectFirstTabsIn == 0)
        selectFirstTabs();
    // Covers the viewport's work area, which excludes the menu bar, the
    // toolbar and the status bar.
    ImGui::DockSpaceOverViewport(dockspaceId, viewport);
}

void EditorLayout::buildDefaultLayout(ImGuiID dockspaceId, ImVec2 size)
{
    ImGui::DockBuilderRemoveNode(dockspaceId);
    ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspaceId, size);

    // Each split takes a slice off the remaining node; what is left in the
    // middle is the center area.
    ImGuiID center = dockspaceId;
    ImGuiID left = ImGui::DockBuilderSplitNode(center, ImGuiDir_Left, _leftRatio, nullptr, &center);
    const ImGuiID leftBottom = ImGui::DockBuilderSplitNode(left, ImGuiDir_Down, LeftBottomRatio, nullptr, &left);
    const ImGuiID right =
        ImGui::DockBuilderSplitNode(center, ImGuiDir_Right, _rightRatio / (1.0f - _leftRatio), nullptr, &center);
    ImGuiID bottom = ImGui::DockBuilderSplitNode(center, ImGuiDir_Down, _bottomRatio, nullptr, &center);

    // The controls strip is cut off the top of the bottom area, just tall
    // enough for one row of large buttons.
    ImGuiID controls = bottom;
    const bool hasControls = std::ranges::any_of(_defaultAreas, [](const auto &entry) { return entry.second == Area::Controls; });
    if (hasControls) {
        const float strip = ImGui::GetFrameHeight() * 1.6f + ImGui::GetStyle().WindowPadding.y * 2.0f;
        const float bottomHeight = size.y * _bottomRatio;
        controls = ImGui::DockBuilderSplitNode(bottom, ImGuiDir_Up, std::min(strip / bottomHeight, 0.5f), nullptr, &bottom);
    }

    for (const auto &[name, area] : _defaultAreas) {
        const ImGuiID node = area == Area::Left ? left
            : area == Area::LeftBottom           ? leftBottom
            : area == Area::Right                ? right
            : area == Area::Bottom               ? bottom
            : area == Area::Controls             ? controls
                                                 : center;
        ImGui::DockBuilderDockWindow(name.c_str(), node);
    }
    ImGui::DockBuilderFinish(dockspaceId);
    _selectFirstTabsIn = 2;
}

void EditorLayout::selectFirstTabs() const
{
    // ImGui selects the last tab docked in a node; the first panel declared
    // for an area is the one meant to be shown. Focusing selects its tab.
    for (const Area area : {Area::Left, Area::LeftBottom, Area::Right, Area::Bottom, Area::Center}) {
        for (const auto &[name, panelArea] : _defaultAreas) {
            if (panelArea == area) {
                if (ImGui::FindWindowByName(name.c_str()))
                    ImGui::SetWindowFocus(name.c_str());
                break;
            }
        }
    }
}
