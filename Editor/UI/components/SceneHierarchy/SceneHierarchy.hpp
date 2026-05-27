/*
    Filename: SceneHierarchy.hpp
    Description: Left-side panel listing all entities, with creation, deletion,
                 duplication, component editing, and per-system management UIs.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚════╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#ifndef PROTO_UI_SCENEHIERARCHY_HPP
#define PROTO_UI_SCENEHIERARCHY_HPP

#include <map>
#include <memory>
#include <optional>
#include <set>
#include <vector>

#include <imgui.h>
#include "imfilebrowser.h"

#include "Entity/GameEntities.hpp"
#include "ECS.hpp"
#include "Scripting/ScriptEngine.hpp"

#include "../common/Panel/Panel.hpp"

namespace Editor::UI::Pages { class EditorPage; }

namespace Editor::UI::Components {

    class SceneHierarchy : public Panel {
    public:
        explicit SceneHierarchy(Pages::EditorPage *page);
        ~SceneHierarchy() override = default;

        void update(ECS::registry &reg) override;
        void render(std::vector<ECS::System::ISystem *> systems) override;

        [[nodiscard]] int getExtraFlags() const override;
        [[nodiscard]] const char *getName() const override { return "Project"; }

    private:
        Pages::EditorPage *_page;

        std::vector<std::shared_ptr<IComponent>> _components;
        bool _showComponentSelectionPopup = false;
        char _entityNameBuffer[256] = "Entity";
        bool _showSystemManagerPopup = false;
        bool _showDuplicatePopup = false;
        std::optional<ECS::Entity::entity> _entityToDuplicate;
        bool _showAddComponentsPopup = false;
        bool _isEditingComponents = false;
        ECS::Entity::entity _pendingEntity;
        std::set<std::string> _selectedComponents;
        ImGui::FileBrowser _scriptFileBrowser;

        void entityButton();
        void renderComponentSelectionPopup(ECS::registry &reg);
        void renderAddComponentsPopup(ECS::registry &reg);
        void renderDuplicatePopup(ECS::registry &reg);
        void renderSystemManagerPopup(std::map<ECS::System::ISystem *, bool> &appliedSystems);
        void displayAvailableEntities(ECS::registry &reg);
        bool renderEntityContextMenu(const ECS::Entity::entity &entity, ECS::registry &reg);
        void renderAttachScriptPopup(ECS::registry &reg);
        void manageSystemsButtons(std::map<ECS::System::ISystem *, bool> &appliedSystems);

        std::optional<ECS::Entity::entity> _contextMenuEntity;
        bool _showAttachScriptPopup = false;
        char _scriptPathBuffer[512] = {};
        char _scriptNsBuffer[128] = "Metrovy";
        char _scriptClassBuffer[128] = "MyScript";
    };

}

#endif
