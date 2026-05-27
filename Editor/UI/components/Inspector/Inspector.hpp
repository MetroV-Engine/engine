/*
    Filename: Inspector.hpp
    Description: Right-side panel showing components of the selected entity.
                 Renders specialised renderers (Script, Bounds, Velocity,
                 Controllable) when available, falls back to generic reflection.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚════╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#ifndef PROTO_UI_INSPECTOR_HPP
#define PROTO_UI_INSPECTOR_HPP

#include <set>
#include <string>

#include <imgui.h>
#include "imfilebrowser.h"

#include "IComponent.hpp"
#include "ComponentRegistry.hpp"
#include "Reflection/FieldDescriptor.hpp"
#include "Entity/Base/Script/ScriptComponent.hpp"

#include "../common/Panel/Panel.hpp"
#include "../../providers/Selection/SelectionProvider.hpp"

namespace Editor::UI::Components {

    class Inspector : public Panel {
    public:
        Inspector();
        ~Inspector() override = default;

        void update(ECS::registry &reg) override;
        void render(std::vector<ECS::System::ISystem *> systems) override;

        [[nodiscard]] bool isVisible() const override
        {
            return Providers::useSelection().hasSelection();
        }
        [[nodiscard]] bool isOverlay() const override { return true; }
        [[nodiscard]] int  getExtraFlags() const override { return 0; }
        [[nodiscard]] const char *getName() const override { return "Inspector"; }

    private:
        bool _showAddComponentPopup = false;
        std::set<std::string> _selectedComponents;
        char _componentNameBuffer[256] = {};
        ECS::registry *_reg = nullptr;
        ImGui::FileBrowser _scriptFileBrowser;

        bool  _preserveRatio = false;
        float _storedRatio   = 1.f;

        static void renderNoSelection();
        void renderEntityHeader(const ECS::Entity::entity &entity);
        void renderAllComponents(const ECS::Entity::entity &entity);
        void renderComponent(IComponent &component);
        static void renderField(ECS::Reflection::FieldDescriptor &field);
        static void renderCloseButton();
        void renderAddComponentButton(Providers::SelectionProvider &sel);
    };

}

#endif
