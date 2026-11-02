/*
    Created on: 30/03/2026
    Filename: HierarchyPanel.cpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "HierarchyPanel.hpp"

#include <cfloat>

HierarchyPanel::HierarchyPanel() : Panel("Hierarchy"), _search("##HierarchySearch", "Search entities")
{
    _search.setWidth(-FLT_MIN);
}

void HierarchyPanel::drawContent()
{
    _search.render();
    ImGui::Separator();
    ImGui::TextDisabled("No world loaded.");
    ImGui::PushTextWrapPos(0.0f);
    ImGui::TextDisabled("Entities will appear here once the engine's ECS is connected.");
    ImGui::PopTextWrapPos();
}
