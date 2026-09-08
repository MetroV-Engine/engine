/*
    Filename: EditorPage.hpp
    Description: Top-level "page" composing the four main editor panels.
                 Responsible for dockspace setup, computing per-panel rectangles
                 (via UseWindowLayout) and calling each panel's update + render.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚════╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#ifndef PROTO_UI_PAGES_EDITORPAGE_HPP
#define PROTO_UI_PAGES_EDITORPAGE_HPP

#include <map>
#include <memory>
#include <vector>

#include "Registry.hpp"
#include "System/ISystem.hpp"

#include "../../components/common/Panel/Panel.hpp"

namespace Editor::UI::Pages {

    class EditorPage {
    public:
        EditorPage();
        ~EditorPage();

        // Add a panel to the page. Panels are rendered in registration order.
        void addPanel(std::shared_ptr<Components::Panel> panel);

        // Render all panels for the current frame.
        void render(ECS::registry &reg,
                    std::vector<ECS::System::ISystem *> &systems);

        // Allow Toolbar (Play button) to know which systems were toggled on.
        void setSystemEnabled(ECS::System::ISystem *sys, bool on) { _enabled[sys] = on; }
        const std::map<ECS::System::ISystem *, bool> &enabledSystems() const { return _enabled; }

    private:
        void renderDockspace(float x, float y, float w, float h);
        void renderPanel(const std::shared_ptr<Components::Panel> &panel,
                         ECS::registry &reg,
                         std::vector<ECS::System::ISystem *> &systems);

        std::vector<std::shared_ptr<Components::Panel>> _panels;
        std::map<ECS::System::ISystem *, bool> _enabled;
    };

}

#endif
