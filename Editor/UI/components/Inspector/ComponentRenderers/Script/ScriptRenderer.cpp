#include "ScriptRenderer.hpp"
#include <imgui.h>
#include <fstream>
#include "ProjectContext.hpp"

namespace Editor::UI::Components::Renderers {
    void ScriptRenderer::render(ECS::Entity::ScriptComponent &script, ImGui::FileBrowser &fileBrowser)
    {
        const bool open = ImGui::CollapsingHeader("##ScriptHeader",
                                                   ImGuiTreeNodeFlags_DefaultOpen |
                                                   ImGuiTreeNodeFlags_AllowOverlap);

        ImGui::SameLine();
        if (script.active) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 1.f, 0.4f, 1.f));
            ImGui::Text(" C#  Script  [active]");
        } else {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.f));
            ImGui::Text(" C#  Script  [inactive]");
        }
        ImGui::PopStyleColor();

        if (!open) return;

        ImGui::Spacing();
        ImGui::TextDisabled("Script Path");
        {
            char buf[512] = {};
            const std::size_t len = std::min(script.scriptPath.size(), sizeof(buf) - 1);
            std::memcpy(buf, script.scriptPath.c_str(), len);
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 105);
            if (ImGui::InputText("##scriptPath", buf, sizeof(buf)))
                script.scriptPath = buf;
            
            ImGui::SameLine();
            if (ImGui::Button("Browse")) {
                fileBrowser.Open();
            }

            if (fileBrowser.HasSelected()) {
                auto selected = fileBrowser.GetSelected();
                script.scriptPath = selected.string();
                fileBrowser.ClearSelected();
                
                std::ifstream file(selected);
                if (file.is_open()) {
                    std::string line;
                    std::string detectedNamespace;
                    std::vector<std::string> detectedClasses;
                    
                    while (std::getline(file, line)) {
                        if (line.find("namespace ") != std::string::npos) {
                            size_t start = line.find("namespace ") + 10;
                            size_t end = line.find_first_of(" {;", start);
                            if (end != std::string::npos) {
                                detectedNamespace = line.substr(start, end - start);
                            }
                        }
                        if (line.find("public class ") != std::string::npos) {
                            size_t start = line.find("public class ") + 13;
                            size_t end = line.find_first_of(" :{", start);
                            if (end != std::string::npos) {
                                detectedClasses.push_back(line.substr(start, end - start));
                            }
                        }
                    }
                    
                    if (!detectedNamespace.empty()) {
                        script.namespaceName = detectedNamespace;
                    }
                    if (detectedClasses.size() == 1) {
                        script.className = detectedClasses[0];
                    }
                }
            }

            if (ImGui::IsItemHovered()) {
                const auto abs = Editor::ProjectContext::getInstance().resolve(script.scriptPath);
                ImGui::SetTooltip("Resolved: %s", abs.string().c_str());
            }
        }

        ImGui::Spacing();
        ImGui::TextDisabled("Namespace");
        {
            char buf[128] = {};
            const std::size_t len = std::min(script.namespaceName.size(), sizeof(buf) - 1);
            std::memcpy(buf, script.namespaceName.c_str(), len);
            ImGui::SetNextItemWidth(-1);
            if (ImGui::InputText("##namespace", buf, sizeof(buf)))
                script.namespaceName = buf;
        }

        ImGui::Spacing();
        ImGui::TextDisabled("Class Name");
        {
            char buf[128] = {};
            const std::size_t len = std::min(script.className.size(), sizeof(buf) - 1);
            std::memcpy(buf, script.className.c_str(), len);
            ImGui::SetNextItemWidth(-1);
            if (ImGui::InputText("##className", buf, sizeof(buf)))
                script.className = buf;
        }

        ImGui::Spacing();
        ImGui::Checkbox("Active", &script.active);
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
    }
}
