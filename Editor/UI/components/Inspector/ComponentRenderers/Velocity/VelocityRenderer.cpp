#include "VelocityRenderer.hpp"
#include <imgui.h>

namespace Editor::UI::Components::Renderers {
    void VelocityRenderer::render(IComponent &component)
    {
        if (!ImGui::CollapsingHeader("Velocity", ImGuiTreeNodeFlags_DefaultOpen))
            return;

        auto fields = component.getFields();
        float *vx = nullptr;
        float *vy = nullptr;
        float *speed = nullptr;
        float *speedX = nullptr;
        float *speedY = nullptr;

        for (auto &field : fields) {
            if (field.name == "vx")
                vx = std::get<float*>(field.valuePtr);
            else if (field.name == "vy")
                vy = std::get<float*>(field.valuePtr);
            else if (field.name == "speed")
                speed = std::get<float*>(field.valuePtr);
            else if (field.name == "speedX")
                speedX = std::get<float*>(field.valuePtr);
            else if (field.name == "speedY")
                speedY = std::get<float*>(field.valuePtr);
        }

        if (vx) ImGui::DragFloat("vx", vx, 0.1f);
        if (vy) ImGui::DragFloat("vy", vy, 0.1f);

        if (speed && speedX && speedY) {
            bool speedsMatch = (*speedX == *speedY);
            
            if (!speedsMatch) {
                ImGui::BeginDisabled();
                ImGui::DragFloat("speed", speed, 0.1f);
                ImGui::EndDisabled();
                if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                    ImGui::SetTooltip("Disabled: speedX and speedY differ");
            } else {
                if (ImGui::DragFloat("speed", speed, 0.1f)) {
                    *speedX = *speed;
                    *speedY = *speed;
                }
            }

            if (ImGui::TreeNode("Advanced Speed")) {
                if (ImGui::DragFloat("speedX", speedX, 0.1f)) {
                    if (*speedX == *speedY)
                        *speed = *speedX;
                }
                if (ImGui::DragFloat("speedY", speedY, 0.1f)) {
                    if (*speedX == *speedY)
                        *speed = *speedY;
                }
                ImGui::TreePop();
            }
        }
    }
}
