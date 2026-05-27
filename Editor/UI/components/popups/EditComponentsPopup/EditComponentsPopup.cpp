#include "EditComponentsPopup.hpp"
#include <imgui.h>
#include "ComponentRegistry.hpp"
#include "IComponent.hpp"

namespace Editor::UI::Components::Popups {
    bool EditComponentsPopup::render(
        bool &showPopup,
        ECS::Entity::entity entity,
        ECS::registry &registry,
        std::set<std::string> &selectedComponents,
        bool isEditing
    ) {
        if (!showPopup)
            return false;

        const char* popupTitle = isEditing ? "Edit Components" : "Add Components";
        ImGui::OpenPopup(popupTitle);
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        if (ImGui::BeginPopupModal(popupTitle, &showPopup, ImGuiWindowFlags_AlwaysAutoResize)) {
            if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                showPopup = false;
                selectedComponents.clear();
                ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
                return false;
            }
            
            ImGui::Text("Select Components");
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::BeginChild("##ComponentList", ImVec2(400, 300), true);
            
            auto &componentRegistry = ECS::Component::ComponentRegistry::getInstance();
            auto componentNames = componentRegistry.getComponentNames();

            for (const auto &componentName : componentNames) {
                bool isSelected = selectedComponents.count(componentName) > 0;
                
                if (ImGui::Selectable(componentName.c_str(), isSelected, ImGuiSelectableFlags_DontClosePopups)) {
                    if (isSelected) {
                        selectedComponents.erase(componentName);
                    } else {
                        selectedComponents.insert(componentName);
                    }
                }
            }
            
            ImGui::EndChild();
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            if (ImGui::Button("OK", ImVec2(120, 0))) {
                auto currentComponents = registry.get_components_for(static_cast<std::size_t>(entity));
                std::set<std::string> currentNames;
                for (auto* comp : currentComponents) {
                    if (comp) currentNames.insert(const_cast<IComponent*>(comp)->getName());
                }
                
                for (const auto& componentName : selectedComponents) {
                    if (currentNames.find(componentName) == currentNames.end()) {
                        componentRegistry.addComponentToEntity(componentName, registry, entity);
                    }
                }
                
                for (const auto& currentName : currentNames) {
                    if (selectedComponents.find(currentName) == selectedComponents.end()) {
                        componentRegistry.removeComponentFromEntity(currentName, registry, entity);
                    }
                }
                
                showPopup = false;
                selectedComponents.clear();
                ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
                return true;
            }

            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                showPopup = false;
                selectedComponents.clear();
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
        return false;
    }
}
