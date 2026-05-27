/*
    Filename: Button.hpp
    Description: Reusable button primitive with semantic variants. Style is
                 pulled from ThemeProvider so visuals stay consistent across panels.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚════╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#ifndef PROTO_UI_COMMON_BUTTON_HPP
#define PROTO_UI_COMMON_BUTTON_HPP

#include <string>
#include <imgui.h>
#include "Types.hpp"

namespace Editor::UI::Components {

    class Button {
    public:
        // Returns true when clicked, like ImGui::Button.
        static bool render(const std::string &label,
                           const ButtonProps &props = {});
    };

}

#endif
