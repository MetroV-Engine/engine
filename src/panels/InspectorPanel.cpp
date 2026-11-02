/*
    Created on: 30/03/2026
    Filename: InspectorPanel.cpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "InspectorPanel.hpp"

void InspectorPanel::drawContent()
{
    if (!_selection.has() || !_model.exists(_selection.get())) {
        ImGui::TextDisabled("No object selected.");
        ImGui::PushTextWrapPos(0.0f);
        ImGui::TextDisabled("Select an object in the Hierarchy to see its details.");
        ImGui::PopTextWrapPos();
        return;
    }

    const HierarchyModel::ObjectId id = _selection.get();
    ImGui::TextUnformatted(_model.name(id).c_str());
    ImGui::Separator();
    if (ImGui::BeginTable("##ObjectInfo", 2)) {
        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
        const HierarchyModel::ObjectId parent = _model.parent(id);
        const auto row = [](const char *label, const std::string &value) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextDisabled("%s", label);
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(value.c_str());
        };
        row("Id", std::to_string(id));
        row("Parent", parent == HierarchyModel::InvalidId ? "(root)" : _model.name(parent));
        row("Children", std::to_string(_model.children(id).size()));
        ImGui::EndTable();
    }
    ImGui::Spacing();
    ImGui::PushTextWrapPos(0.0f);
    ImGui::TextDisabled("Components will be listed here once the engine's ECS is connected.");
    ImGui::PopTextWrapPos();
}
