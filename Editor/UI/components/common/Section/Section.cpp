#include "Section.hpp"

#include <imgui.h>

namespace Editor::UI::Components {

    bool Section::begin(const std::string &title)
    {
        return ImGui::CollapsingHeader(title.c_str(),
                                       ImGuiTreeNodeFlags_DefaultOpen);
    }

    bool Section::renderWithRemove(const std::string &title,
                                   const std::function<void()> &body)
    {
        bool removeClicked = false;
        if (ImGui::CollapsingHeader(title.c_str(),
                                    ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::SameLine(ImGui::GetContentRegionAvail().x - 18.0f);
            ImGui::PushID((title + "##rm").c_str());
            if (ImGui::SmallButton("x")) removeClicked = true;
            ImGui::PopID();
            if (body) body();
        } else {
            ImGui::SameLine(ImGui::GetContentRegionAvail().x - 18.0f);
            ImGui::PushID((title + "##rm").c_str());
            if (ImGui::SmallButton("x")) removeClicked = true;
            ImGui::PopID();
        }
        return removeClicked;
    }

}
