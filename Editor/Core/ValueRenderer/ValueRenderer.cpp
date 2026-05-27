#include "ValueRenderer.hpp"
#include <imgui.h>
#include <cstring>

namespace Editor {
    void ValueRenderer::renderField(const std::string &name, const ECS::Reflection::FieldType type,
                                    const ECS::Reflection::FieldValue value)
    {
        auto it = _renderingMap.find(type);
        if (it != _renderingMap.end()) it->second(name, value);
    }

    ValueRenderer::ValueRenderer()
    {
        _renderingMap[ECS::Reflection::FieldType::Float] = [](const std::string &name, ECS::Reflection::FieldValue value) {
            renderFloat(name, std::get<float *>(value));
        };
        _renderingMap[ECS::Reflection::FieldType::Int] = [](const std::string &name, ECS::Reflection::FieldValue value) {
            renderInt(name, std::get<int *>(value));
        };
        _renderingMap[ECS::Reflection::FieldType::Bool] = [](const std::string &name, ECS::Reflection::FieldValue value) {
            renderBool(name, std::get<bool *>(value));
        };
        _renderingMap[ECS::Reflection::FieldType::Vec2f] = [](const std::string &name, ECS::Reflection::FieldValue value) {
            renderVec2f(name, std::get<Proto::Vec2f *>(value));
        };
        _renderingMap[ECS::Reflection::FieldType::String] = [](const std::string &name, ECS::Reflection::FieldValue value) {
            renderString(name, std::get<std::string *>(value));
        };
        _renderingMap[ECS::Reflection::FieldType::KeyMapLayout] = [](const std::string &name, ECS::Reflection::FieldValue value) {
            renderKeyMapLayout(name, std::get<std::map<ECS::Entity::Moves, Proto::Key> *>(value));
        };
        _renderingMap[ECS::Reflection::FieldType::Color] = [](const std::string &name, ECS::Reflection::FieldValue value) {
            renderColor(name, std::get<Proto::Color *>(value));
        };
    }

    void ValueRenderer::renderFloat(const std::string &name, float *value) { ImGui::DragFloat(name.c_str(), value, 0.1f); }
    void ValueRenderer::renderInt(const std::string &name, int *value) { ImGui::DragInt(name.c_str(), value); }
    void ValueRenderer::renderBool(const std::string &name, bool *value) { ImGui::Checkbox(name.c_str(), value); }

    void ValueRenderer::renderVec2f(const std::string &name, Proto::Vec2f *value)
    {
        float arr[2] = {value->x, value->y};
        if (ImGui::DragFloat2(name.c_str(), arr, 0.1f)) { value->x = arr[0]; value->y = arr[1]; }
    }

    void ValueRenderer::renderString(const std::string &name, std::string *value)
    {
        char buf[512] = {};
        std::memcpy(buf, value->c_str(), std::min(value->size(), sizeof(buf) - 1));
        const std::string label = "##str_" + name;
        ImGui::SetNextItemWidth(-1);
        if (ImGui::InputText(label.c_str(), buf, sizeof(buf))) *value = buf;
    }

    ECS::Entity::Moves ValueRenderer::_recordingMove = ECS::Entity::Moves::UP;
    bool ValueRenderer::_isRecording = false;

    void ValueRenderer::renderKeyMapLayout(const std::string &name, std::map<ECS::Entity::Moves, Proto::Key> *value)
    {
        if (ImGui::TreeNode(name.c_str())) {
            static const std::map<ECS::Entity::Moves, const char *> moveNames = {
                {ECS::Entity::Moves::UP, "UP"}, {ECS::Entity::Moves::DOWN, "DOWN"},
                {ECS::Entity::Moves::LEFT, "LEFT"}, {ECS::Entity::Moves::RIGHT, "RIGHT"}
            };
            
            for (auto &[move, key]: *value) {
                auto moveNameIt = moveNames.find(move);
                const char *moveName = (moveNameIt != moveNames.end()) ? moveNameIt->second : "Unknown";
                
                ImGui::Text("%s:", moveName);
                ImGui::SameLine();
                
                int currentKeyIndex = 0;
                for (int i = 0; i < (int)(sizeof(keyValues) / sizeof(keyValues[0])); ++i) {
                    if (keyValues[i] == key) { currentKeyIndex = i; break; }
                }
                
                std::string comboLabel = "##combo_" + name + std::to_string(static_cast<int>(move));
                ImGui::SetNextItemWidth(150);
                if (ImGui::Combo(comboLabel.c_str(), &currentKeyIndex, keyNames, sizeof(keyNames) / sizeof(keyNames[0])))
                    key = keyValues[currentKeyIndex];
                
                ImGui::SameLine();
                std::string recordLabel = (_isRecording && _recordingMove == move) ? "Recording..." : "Record";
                std::string buttonLabel = recordLabel + "##record_" + name + std::to_string(static_cast<int>(move));
                
                if (_isRecording && _recordingMove == move) {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.3f, 0.3f, 1.0f));
                    if (ImGui::Button(buttonLabel.c_str())) {
                        _isRecording = true;
                        _recordingMove = move;
                    }
                    ImGui::PopStyleColor();
                    
                    for (int i = 0; i < (int)(sizeof(keyValues) / sizeof(keyValues[0])); ++i) {
                        if (Proto::isKeyPressed(keyValues[i])) {
                            key = keyValues[i];
                            _isRecording = false;
                            break;
                        }
                    }
                } else {
                    if (ImGui::Button(buttonLabel.c_str())) {
                        _isRecording = true;
                        _recordingMove = move;
                    }
                }
            }
            ImGui::TreePop();
        }
    }

    void ValueRenderer::renderColor(const std::string &name, Proto::Color *value)
    {
        float color[4] = {value->r / 255.f, value->g / 255.f, value->b / 255.f, value->a / 255.f};
        if (ImGui::ColorEdit4(name.c_str(), color)) {
            value->r = static_cast<std::uint8_t>(color[0] * 255);
            value->g = static_cast<std::uint8_t>(color[1] * 255);
            value->b = static_cast<std::uint8_t>(color[2] * 255);
            value->a = static_cast<std::uint8_t>(color[3] * 255);
        }
    }
}
