#include "SceneHierarchy.hpp"

#include <cstring>
#include <fstream>
#include <iostream>

#include "Entity/Base/Position/Position.hpp"
#include "Entity/Base/Velocity/Velocity.hpp"
#include "Entity/Base/Bounds/Bounds.hpp"
#include "Entity/Base/Drawable/Drawable.hpp"
#include "Entity/Base/Controlable/Controlable.hpp"

#include "ComponentRegistry.hpp"

#include "../../providers/Selection/SelectionProvider.hpp"
#include "../../providers/Scale/ScaleProvider.hpp"
#include "../../pages/Editor/EditorPage.hpp"
#include "../popups/EditComponentsPopup/EditComponentsPopup.hpp"

namespace Editor::UI::Components {

    SceneHierarchy::SceneHierarchy(Pages::EditorPage *page) : _page(page)
    {
        _components.push_back(std::make_shared<ECS::Entity::Position>());
        _components.push_back(std::make_shared<ECS::Entity::Velocity>());

        _scriptFileBrowser.SetTitle("Select Script");
        _scriptFileBrowser.SetTypeFilters({ ".cs" });
        _scriptFileBrowser.SetPwd(std::filesystem::path(PROJECT_ROOT_DIR) / "scripts");
    }

    int SceneHierarchy::getExtraFlags() const
    {
        return ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
               ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking;
    }

    void SceneHierarchy::render(std::vector<ECS::System::ISystem *> systems)
    {
        (void)systems;
    }

    void SceneHierarchy::update(ECS::registry &reg)
    {
        if (ImGui::CollapsingHeader("Entities", ImGuiTreeNodeFlags_DefaultOpen)) {
            displayAvailableEntities(reg);
        }

        ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

        if (ImGui::CollapsingHeader("Actions", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Spacing();
            entityButton();
            ImGui::Spacing();
            if (_page) manageSystemsButtons(const_cast<std::map<ECS::System::ISystem *, bool>&>(_page->enabledSystems()));
            ImGui::Spacing();
        }

        renderComponentSelectionPopup(reg);
        renderAddComponentsPopup(reg);
        renderDuplicatePopup(reg);

        _scriptFileBrowser.Display();
        if (_scriptFileBrowser.HasSelected()) {
            auto selected = _scriptFileBrowser.GetSelected();
            std::strncpy(_scriptPathBuffer, selected.string().c_str(), sizeof(_scriptPathBuffer) - 1);
            _scriptPathBuffer[sizeof(_scriptPathBuffer) - 1] = '\0';

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
                    std::strncpy(_scriptNsBuffer, detectedNamespace.c_str(), sizeof(_scriptNsBuffer) - 1);
                    _scriptNsBuffer[sizeof(_scriptNsBuffer) - 1] = '\0';
                }
                if (detectedClasses.size() == 1) {
                    std::strncpy(_scriptClassBuffer, detectedClasses[0].c_str(), sizeof(_scriptClassBuffer) - 1);
                    _scriptClassBuffer[sizeof(_scriptClassBuffer) - 1] = '\0';
                }
            }

            _scriptFileBrowser.ClearSelected();
        }
    }

    void SceneHierarchy::entityButton()
    {
        const Proto::Vec2f scale = Providers::useScale().getScale();
        const auto buttonSize = ImVec2(-1, 50 * scale.y);

        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.25f, 0.55f, 0.85f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f, 0.65f, 0.95f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.20f, 0.50f, 0.80f, 1.0f));

        if (ImGui::Button("+ Create Entity", buttonSize)) {
            _showComponentSelectionPopup = true;
            ImGui::OpenPopup("Select Component");
        }

        ImGui::PopStyleColor(3);
    }

    void SceneHierarchy::renderComponentSelectionPopup(ECS::registry &reg)
    {
        if (!_showComponentSelectionPopup) return;

        ImGui::OpenPopup("Select Component");
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        if (ImGui::BeginPopupModal("Select Component", &_showComponentSelectionPopup, ImGuiWindowFlags_AlwaysAutoResize)) {
            if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                _showComponentSelectionPopup = false;
                ImGui::CloseCurrentPopup();
            }

            ImGui::Text("Create New Entity");
            ImGui::Separator(); ImGui::Spacing();

            ImGui::Text("Entity Name:");
            ImGui::SetNextItemWidth(300);
            ImGui::InputText("##EntityName", _entityNameBuffer, sizeof(_entityNameBuffer));
            ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

            if (ImGui::Button("OK", ImVec2(120, 0))) {
                _pendingEntity = reg.spawn_entity();
                reg.set_entity_name(_pendingEntity, _entityNameBuffer);
                _showComponentSelectionPopup = false;
                _showAddComponentsPopup = true;
                _isEditingComponents = false;
                _selectedComponents.clear();
                ImGui::CloseCurrentPopup();
            }

            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                _showComponentSelectionPopup = false;
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }

    void SceneHierarchy::renderAddComponentsPopup(ECS::registry &reg)
    {
        if (!_showAddComponentsPopup) return;
        Editor::UI::Components::Popups::EditComponentsPopup::render(_showAddComponentsPopup, _pendingEntity, reg, _selectedComponents, _isEditingComponents);
    }

    void SceneHierarchy::renderDuplicatePopup(ECS::registry &reg)
    {
        if (!_showDuplicatePopup) return;

        ImGui::OpenPopup("Duplicate Entity");
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        if (ImGui::BeginPopupModal("Duplicate Entity", &_showDuplicatePopup, ImGuiWindowFlags_AlwaysAutoResize)) {
            if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                _showDuplicatePopup = false;
                ImGui::CloseCurrentPopup();
            }

            ImGui::Text("Duplicate Entity");
            ImGui::Separator(); ImGui::Spacing();

            ImGui::Text("New Entity Name:");
            ImGui::SetNextItemWidth(300);
            ImGui::InputText("##DuplicateName", _entityNameBuffer, sizeof(_entityNameBuffer));
            ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

            if (ImGui::Button("OK", ImVec2(120, 0))) {
                if (_entityToDuplicate.has_value()) {
                    auto srcEntity = _entityToDuplicate.value();
                    auto newEntity = reg.spawn_entity();
                    reg.set_entity_name(newEntity, _entityNameBuffer);

                    auto* positions = reg.get_if<ECS::Entity::Position>();
                    if (positions && positions->has(static_cast<std::size_t>(srcEntity))) {
                        auto srcPos = positions->get(static_cast<std::size_t>(srcEntity));
                        reg.add_component(newEntity, ECS::Entity::Position(srcPos._x + 20, srcPos._y + 20));
                    }
                    auto* velocities = reg.get_if<ECS::Entity::Velocity>();
                    if (velocities && velocities->has(static_cast<std::size_t>(srcEntity))) {
                        auto srcVel = velocities->get(static_cast<std::size_t>(srcEntity));
                        reg.add_component(newEntity, ECS::Entity::Velocity(srcVel));
                    }
                    auto* bounds = reg.get_if<ECS::Entity::Bounds>();
                    if (bounds && bounds->has(static_cast<std::size_t>(srcEntity))) {
                        auto srcBounds = bounds->get(static_cast<std::size_t>(srcEntity));
                        reg.add_component(newEntity, ECS::Entity::Bounds(srcBounds));
                    }
                    auto* drawables = reg.get_if<ECS::Entity::Drawable>();
                    if (drawables && drawables->has(static_cast<std::size_t>(srcEntity))) {
                        auto srcDrawable = drawables->get(static_cast<std::size_t>(srcEntity));
                        reg.add_component(newEntity, ECS::Entity::Drawable(srcDrawable));
                    }
                    auto* controllables = reg.get_if<ECS::Entity::Controlable>();
                    if (controllables && controllables->has(static_cast<std::size_t>(srcEntity))) {
                        auto srcCtrl = controllables->get(static_cast<std::size_t>(srcEntity));
                        reg.add_component(newEntity, ECS::Entity::Controlable(srcCtrl));
                    }

                    Providers::useSelection().selectEntity(newEntity);
                }
                _showDuplicatePopup = false;
                ImGui::CloseCurrentPopup();
            }

            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                _showDuplicatePopup = false;
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }

    void SceneHierarchy::displayAvailableEntities(ECS::registry &reg)
    {
        auto &sel = Providers::useSelection();

        ImGui::BeginChild("##EntityList", ImVec2(0, 300), true);

        std::vector<ECS::Entity::entity> entities = reg.get_all_entities();
        for (const auto &entity : entities) {
            const bool isSelected = sel.hasSelection() && sel.getSelectedEntity() == entity;
            const std::string label = entity.getName() + "##" + std::to_string(static_cast<std::size_t>(entity));

            if (ImGui::Selectable(label.c_str(), isSelected))
                sel.selectEntity(entity);

            if (renderEntityContextMenu(entity, reg))
                break;
        }

        if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
            !ImGui::IsAnyItemHovered()) {
            sel.clearSelection();
        }

        ImGui::EndChild();

        if (_showAttachScriptPopup)
            renderAttachScriptPopup(reg);
    }

    bool SceneHierarchy::renderEntityContextMenu(const ECS::Entity::entity &entity, ECS::registry &reg)
    {
        auto &sel = Providers::useSelection();
        auto &eng = Editor::Scripting::ScriptEngine::getInstance();

        const std::string ctxId = "entity_ctx##" + std::to_string(static_cast<std::size_t>(entity));

        if (!ImGui::BeginPopupContextItem(ctxId.c_str()))
            return false;

        _contextMenuEntity = entity;

        ImGui::TextDisabled("Entity: %s  [id %zu]",
            entity.getName().empty() ? "Unnamed" : entity.getName().c_str(),
            static_cast<std::size_t>(entity));
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::MenuItem("  Select")) {
            sel.selectEntity(entity);
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::MenuItem("  Duplicate")) {
            _entityToDuplicate = entity;
            std::string newName = entity.getName() + " Copy";
            std::strncpy(_entityNameBuffer, newName.c_str(), sizeof(_entityNameBuffer) - 1);
            _entityNameBuffer[sizeof(_entityNameBuffer) - 1] = '\0';
            _showDuplicatePopup = true;
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::MenuItem("  Edit Components")) {
            _pendingEntity = entity;
            _selectedComponents.clear();
            auto components = reg.get_components_for(static_cast<std::size_t>(entity));
            for (auto* comp : components) {
                if (comp) _selectedComponents.insert(const_cast<IComponent*>(comp)->getName());
            }
            _isEditingComponents = true;
            _showAddComponentsPopup = true;
            ImGui::CloseCurrentPopup();
        }

        ImGui::Separator();

        if (eng.hasScript(entity, reg)) {
            if (ImGui::MenuItem("  Edit Script")) {
                sel.selectEntity(entity);
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::MenuItem("  Remove Script")) {
                eng.detachScript(entity, reg);
                ImGui::CloseCurrentPopup();
            }
        } else {
            if (ImGui::MenuItem("  Attach Script")) {
                _showAttachScriptPopup = true;
                std::memset(_scriptPathBuffer,  0, sizeof(_scriptPathBuffer));
                std::memset(_scriptNsBuffer,    0, sizeof(_scriptNsBuffer));
                std::memset(_scriptClassBuffer, 0, sizeof(_scriptClassBuffer));
                std::strncpy(_scriptNsBuffer,    "Metrovy",  sizeof(_scriptNsBuffer)    - 1);
                std::strncpy(_scriptClassBuffer, "MyScript", sizeof(_scriptClassBuffer) - 1);
                ImGui::CloseCurrentPopup();
            }
        }

        ImGui::Separator();

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.3f, 0.3f, 1.f));
        const bool deleted = ImGui::MenuItem("  Delete Entity");
        ImGui::PopStyleColor();

        ImGui::EndPopup();

        if (deleted) {
            if (sel.hasSelection() && sel.getSelectedEntity() == entity)
                sel.clearSelection();
            reg.kill_entity(entity);
            return true;
        }
        return false;
    }

    void SceneHierarchy::renderSystemManagerPopup(std::map<ECS::System::ISystem *, bool> &appliedSystems)
    {
        const Proto::Vec2f scale = Providers::useScale().getScale();
        const ImVec2 popupSize = ImVec2(600 * scale.x, 500 * scale.y);
        ImGui::SetNextWindowSize(popupSize, ImGuiCond_Always);

        if (ImGui::BeginPopupModal("System Manager", &_showSystemManagerPopup, ImGuiWindowFlags_NoResize)) {
            if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                _showSystemManagerPopup = false;
                ImGui::CloseCurrentPopup();
            }

            ImGui::Text("System Management");
            ImGui::Separator(); ImGui::Spacing();
            ImGui::Text("Enable/Disable Systems:"); ImGui::Spacing();

            for (auto& [system, isEnabled] : appliedSystems) {
                if (system) {
                    const char* systemName = system->getName();
                    ImGui::Checkbox(systemName, &isEnabled);
                }
            }

            ImGui::Spacing(); ImGui::Separator();

            const float buttonWidth = 100.0f * scale.x;
            const float windowWidth = ImGui::GetWindowWidth();
            const float startX = (windowWidth - buttonWidth) * 0.5f;

            ImGui::SetCursorPosX(startX);
            if (ImGui::Button("Close", ImVec2(buttonWidth, 0))) {
                _showSystemManagerPopup = false;
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }

    void SceneHierarchy::manageSystemsButtons(std::map<ECS::System::ISystem *, bool> &appliedSystems)
    {
        const Proto::Vec2f scale = Providers::useScale().getScale();
        const auto buttonSize = ImVec2(-1, 50 * scale.y);

        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.55f, 0.35f, 0.75f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.65f, 0.45f, 0.85f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.50f, 0.30f, 0.70f, 1.0f));

        if (ImGui::Button("⚙ Manage Systems", buttonSize)) {
            _showSystemManagerPopup = true;
            ImGui::OpenPopup("System Manager");
        }

        ImGui::PopStyleColor(3);

        if (_showSystemManagerPopup) renderSystemManagerPopup(appliedSystems);
    }

    void SceneHierarchy::renderAttachScriptPopup(ECS::registry &reg)
    {
        const Proto::Vec2f scale = Providers::useScale().getScale();
        ImGui::OpenPopup("Attach Script");
        const ImVec2 popupSize = ImVec2(500 * scale.x, 220 * scale.y);
        ImGui::SetNextWindowSize(popupSize, ImGuiCond_Always);

        if (ImGui::BeginPopupModal("Attach Script", &_showAttachScriptPopup, ImGuiWindowFlags_NoResize)) {
            if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                _showAttachScriptPopup = false;
                ImGui::CloseCurrentPopup();
            }

            if (_contextMenuEntity.has_value()) {
                const auto &entity = _contextMenuEntity.value();
                ImGui::Text("Attach Script to: %s  [id %zu]",
                    entity.getName().empty() ? "Unnamed" : entity.getName().c_str(),
                    static_cast<std::size_t>(entity));
            }
            ImGui::Separator(); ImGui::Spacing();

            ImGui::TextDisabled("Script Path (.cs)");
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 90);
            ImGui::InputText("##ScriptPath", _scriptPathBuffer, sizeof(_scriptPathBuffer));
            ImGui::SameLine();
            if (ImGui::Button("Browse...")) _scriptFileBrowser.Open();

            ImGui::TextDisabled("Namespace");
            ImGui::SetNextItemWidth(-1);
            ImGui::InputText("##ScriptNs", _scriptNsBuffer, sizeof(_scriptNsBuffer));

            ImGui::TextDisabled("Class Name");
            ImGui::SetNextItemWidth(-1);
            ImGui::InputText("##ClassName", _scriptClassBuffer, sizeof(_scriptClassBuffer));

            ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

            const float btnW   = 120.f * scale.x;
            const float startX = (ImGui::GetWindowWidth() - (btnW * 2 + 10.f * scale.x)) * 0.5f;
            ImGui::SetCursorPosX(startX);

            const bool canAttach = _contextMenuEntity.has_value() &&
                                   _scriptPathBuffer[0] != '\0' &&
                                   _scriptClassBuffer[0] != '\0';

            if (!canAttach) ImGui::BeginDisabled();
            if (ImGui::Button("Attach", ImVec2(btnW, 0))) {
                Editor::Scripting::ScriptEngine::getInstance().attachScript(
                    _contextMenuEntity.value(),
                    _scriptPathBuffer,
                    _scriptClassBuffer,
                    _scriptNsBuffer,
                    reg);
                Providers::useSelection().selectEntity(_contextMenuEntity.value());
                _showAttachScriptPopup = false;
                ImGui::CloseCurrentPopup();
            }
            if (!canAttach) ImGui::EndDisabled();

            ImGui::SameLine(0, 10.f * scale.x);
            if (ImGui::Button("Cancel", ImVec2(btnW, 0))) {
                _showAttachScriptPopup = false;
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }

}
