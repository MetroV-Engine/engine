#include "ControllableRenderer.hpp"
#include <imgui.h>
#include "Entity/Base/Controlable/Controlable.hpp"
#include "ValueRenderer/ValueRenderer.hpp"

namespace Editor::UI::Components::Renderers {
    void ControllableRenderer::render(IComponent &component)
    {
        if (!ImGui::CollapsingHeader("Controlable", ImGuiTreeNodeFlags_DefaultOpen))
            return;

        auto fields = component.getFields();
        bool *canMove = nullptr;
        std::map<ECS::Entity::Moves, Proto::Key> *controlsMap = nullptr;

        for (auto &field : fields) {
            if (field.name == "canMove")
                canMove = std::get<bool*>(field.valuePtr);
            else if (field.name == "layout")
                controlsMap = std::get<std::map<ECS::Entity::Moves, Proto::Key>*>(field.valuePtr);
        }

        if (canMove)
            ImGui::Checkbox("canMove", canMove);

        if (controlsMap && ImGui::TreeNode("layout")) {
            renderKeyMapLayout(controlsMap);
            ImGui::TreePop();
        }
    }

    void ControllableRenderer::renderKeyMapLayout(std::map<ECS::Entity::Moves, Proto::Key> *value)
    {
        static const std::map<ECS::Entity::Moves, const char *> moveNames = {
            {ECS::Entity::Moves::UP, "UP"}, {ECS::Entity::Moves::DOWN, "DOWN"},
            {ECS::Entity::Moves::LEFT, "LEFT"}, {ECS::Entity::Moves::RIGHT, "RIGHT"}
        };

        static bool isRecording = false;
        static ECS::Entity::Moves recordingMove = ECS::Entity::Moves::UP;
        static float escHoldTime = 0.0f;

        for (auto &[move, key]: *value) {
            auto moveNameIt = moveNames.find(move);
            const char *moveName = (moveNameIt != moveNames.end()) ? moveNameIt->second : "Unknown";
            
            bool cleared = (key == static_cast<Proto::Key>(0));
            
            ImGui::Text("%s:", moveName);
            ImGui::SameLine();
            
            if (cleared) {
                ImGui::TextDisabled("None");
            } else {
                int currentKeyIndex = 0;
                for (int i = 0; i < (int)(sizeof(Editor::keyValues) / sizeof(Editor::keyValues[0])); ++i) {
                    if (Editor::keyValues[i] == key) { currentKeyIndex = i; break; }
                }
                
                std::string comboLabel = "##combo_layout" + std::to_string(static_cast<int>(move));
                ImGui::SetNextItemWidth(100);
                if (ImGui::Combo(comboLabel.c_str(), &currentKeyIndex, Editor::keyNames, sizeof(Editor::keyNames) / sizeof(Editor::keyNames[0])))
                    key = Editor::keyValues[currentKeyIndex];
            }
            
            ImGui::SameLine();
            std::string recordLabel = (isRecording && recordingMove == move) ? "Recording..." : "Record";
            std::string buttonLabel = recordLabel + "##record_" + std::to_string(static_cast<int>(move));
            
            if (isRecording && recordingMove == move) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.3f, 0.3f, 1.0f));
                if (ImGui::Button(buttonLabel.c_str())) {
                    isRecording = false;
                }
                ImGui::PopStyleColor();
                
                if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                    isRecording = false;
                    escHoldTime = 0.0f;
                } else if (ImGui::IsKeyDown(ImGuiKey_Escape)) {
                    escHoldTime += ImGui::GetIO().DeltaTime;
                    if (escHoldTime > 0.5f) {
                        key = static_cast<Proto::Key>(0);
                        isRecording = false;
                        escHoldTime = 0.0f;
                    }
                } else {
                    escHoldTime = 0.0f;
                    for (int i = 0; i < (int)(sizeof(Editor::keyValues) / sizeof(Editor::keyValues[0])); ++i) {
                        if (Proto::isKeyPressed(Editor::keyValues[i])) {
                            key = Editor::keyValues[i];
                            isRecording = false;
                            break;
                        }
                    }
                }
            } else {
                if (ImGui::Button(buttonLabel.c_str())) {
                    isRecording = true;
                    recordingMove = move;
                    escHoldTime = 0.0f;
                }
            }
            
            ImGui::SameLine();
            std::string clearLabel = "X##clear_" + std::to_string(static_cast<int>(move));
            float h = ImGui::GetFrameHeight();
            ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.6f, 0.1f, 0.1f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
            if (ImGui::Button(clearLabel.c_str(), ImVec2(h, h))) {
                key = static_cast<Proto::Key>(0);
            }
            ImGui::PopStyleColor(3);
        }
    }
}
