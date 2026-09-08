#include "Inspector.hpp"

#include <cstring>
#include <fstream>

#include "SdlTypes.hpp"
#include "ProjectContext.hpp"
#include "ValueRenderer/ValueRenderer.hpp"

#include "ComponentRenderers/Script/ScriptRenderer.hpp"
#include "ComponentRenderers/Bounds/BoundsRenderer.hpp"
#include "ComponentRenderers/Velocity/VelocityRenderer.hpp"
#include "ComponentRenderers/Controllable/ControllableRenderer.hpp"

#include "../popups/EditComponentsPopup/EditComponentsPopup.hpp"

namespace Editor::UI::Components {

    Inspector::Inspector()
    {
        _scriptFileBrowser.SetTitle("Select Script");
        _scriptFileBrowser.SetTypeFilters({ ".cs" });
        _scriptFileBrowser.SetPwd(std::filesystem::path(PROJECT_ROOT_DIR) / "scripts");
    }

    void Inspector::update(ECS::registry &reg)
    {
        _reg = &reg;
        _scriptFileBrowser.Display();
    }

    void Inspector::render(std::vector<ECS::System::ISystem *> systems)
    {
        (void)systems;
        auto &sel = Providers::useSelection();

        renderAddComponentButton(sel);

        if (!sel.hasSelection() || _reg == nullptr) {
            renderNoSelection();
            return;
        }

        const ECS::Entity::entity &entity = sel.getSelectedEntity();
        renderEntityHeader(entity);
        renderAllComponents(entity);
    }

    void Inspector::renderNoSelection()
    {
        const char *hint = "Select an entity";
        ImVec2 sz  = ImGui::GetWindowSize();
        ImVec2 tsz = ImGui::CalcTextSize(hint);
        ImGui::SetCursorPos(ImVec2((sz.x - tsz.x) * 0.5f, (sz.y - tsz.y) * 0.5f));
        ImGui::TextDisabled("%s", hint);
    }

    void Inspector::renderEntityHeader(const ECS::Entity::entity &entity)
    {
        ImGui::Text("Entity: %s  [id %zu]",
                    entity.getName().empty() ? "Unnamed" : entity.getName().c_str(),
                    static_cast<std::size_t>(entity));
        renderCloseButton();
        ImGui::Separator();
        ImGui::Spacing();
    }

    void Inspector::renderCloseButton()
    {
        float closeButtonSize = ImGui::GetFrameHeight();
        float windowWidth = ImGui::GetWindowWidth();

        ImGui::SameLine();
        ImGui::SetCursorPosX(windowWidth - closeButtonSize - ImGui::GetStyle().WindowPadding.x);
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.6f, 0.1f, 0.1f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
        if (ImGui::Button("X##closeInspector", ImVec2(closeButtonSize, closeButtonSize)))
            Providers::useSelection().clearSelection();
        ImGui::PopStyleColor(3);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Deselect entity");
    }

    void Inspector::renderAllComponents(const ECS::Entity::entity &entity)
    {
        auto id = static_cast<std::size_t>(entity);
        for (auto *component : _reg->get_components_for(id))
            renderComponent(*component);
    }

    void Inspector::renderComponent(IComponent &component)
    {
        if (component.getName() == "Script") {
            Editor::UI::Components::Renderers::ScriptRenderer::render(
                dynamic_cast<ECS::Entity::ScriptComponent &>(component), _scriptFileBrowser);
            return;
        }
        if (component.getName() == "Bounds") {
            Editor::UI::Components::Renderers::BoundsRenderer::render(component, _preserveRatio, _storedRatio);
            return;
        }
        if (component.getName() == "Velocity") {
            Editor::UI::Components::Renderers::VelocityRenderer::render(component);
            return;
        }
        if (component.getName() == "Controlable") {
            Editor::UI::Components::Renderers::ControllableRenderer::render(component);
            return;
        }

        if (ImGui::CollapsingHeader(component.getName().c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
            auto fields = component.getFields();
            for (auto &field : fields)
                renderField(field);
        }
    }

    void Inspector::renderField(ECS::Reflection::FieldDescriptor &field)
    {
        auto &valueRenderer = Editor::ValueRenderer::getInstance();
        valueRenderer.renderField(field.name, field.type, field.valuePtr);
    }

    void Inspector::renderAddComponentButton(Providers::SelectionProvider &sel)
    {
        if (sel.hasSelection() && _reg != nullptr) {
            if (ImGui::Button("Edit Components", ImVec2(-1, 0))) {
                const ECS::Entity::entity &entity = sel.getSelectedEntity();
                _selectedComponents.clear();
                auto components = _reg->get_components_for(static_cast<std::size_t>(entity));
                for (auto* comp : components) {
                    if (comp) _selectedComponents.insert(const_cast<IComponent*>(comp)->getName());
                }
                _showAddComponentPopup = true;
                ImGui::OpenPopup("Edit Components");
            }
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
        }

        if (_showAddComponentPopup && sel.hasSelection()) {
            const ECS::Entity::entity &entity = sel.getSelectedEntity();
            Editor::UI::Components::Popups::EditComponentsPopup::render(_showAddComponentPopup, entity, *_reg, _selectedComponents, true);
        }
    }

}
