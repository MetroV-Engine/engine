/*
    Filename: Panel.hpp
    Description: Base class for any windowed editor panel. Replaces the old
                 ILayer / ALayer pair with a single class focused on what panels
                 actually need: a name, an update step, a render step, an
                 imgui-flags hook, and a visibility flag.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚════█══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#ifndef PROTO_UI_COMMON_PANEL_HPP
#define PROTO_UI_COMMON_PANEL_HPP

#include <string>
#include <vector>
#include "Registry.hpp"
#include "System/ISystem.hpp"

namespace Editor::UI::Components {

    class Panel {
    public:
        virtual ~Panel() = default;

        // Called every frame, before render. Stash the registry so render() can
        // use it without parameter passing.
        virtual void update(ECS::registry &reg) = 0;

        // Called every frame, inside the panel's ImGui window scope.
        virtual void render(std::vector<ECS::System::ISystem *> systems) = 0;

        [[nodiscard]] virtual bool isVisible() const  { return true; }
        [[nodiscard]] virtual int  getExtraFlags() const { return 0; }
        [[nodiscard]] virtual bool isOverlay() const  { return false; }
        [[nodiscard]] virtual const char *getName() const = 0;

        void setSystems(const std::vector<ECS::System::ISystem *> &systems) { _systems = systems; }

    protected:
        std::vector<ECS::System::ISystem *> _systems;
    };

}

#endif
