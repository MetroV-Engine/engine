/*
    Filename: Colors.hpp
    Description: Editor color palette. All ImGui style colors are sourced here.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#ifndef PROTO_UI_THEME_COLORS_HPP
#define PROTO_UI_THEME_COLORS_HPP

#include <imgui.h>

namespace Editor::UI::Theme {

    struct Colors {
        ImVec4 windowBg     = ImVec4(0.12f, 0.12f, 0.14f, 1.00f);
        ImVec4 panelBg      = ImVec4(0.12f, 0.12f, 0.14f, 1.00f);
        ImVec4 titleBg      = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);
        ImVec4 titleBgActive= ImVec4(0.14f, 0.14f, 0.18f, 1.00f);
        ImVec4 tab          = ImVec4(0.14f, 0.14f, 0.18f, 1.00f);
        ImVec4 tabSelected  = ImVec4(0.24f, 0.24f, 0.32f, 1.00f);
        ImVec4 tabHovered   = ImVec4(0.32f, 0.32f, 0.42f, 1.00f);
        ImVec4 header       = ImVec4(0.20f, 0.20f, 0.26f, 1.00f);
        ImVec4 headerHov    = ImVec4(0.28f, 0.28f, 0.36f, 1.00f);
        ImVec4 headerActive = ImVec4(0.32f, 0.32f, 0.42f, 1.00f);
        ImVec4 button       = ImVec4(0.20f, 0.20f, 0.26f, 1.00f);
        ImVec4 buttonHov    = ImVec4(0.30f, 0.30f, 0.40f, 1.00f);
        ImVec4 buttonActive = ImVec4(0.36f, 0.36f, 0.48f, 1.00f);
        ImVec4 frameBg      = ImVec4(0.16f, 0.16f, 0.20f, 1.00f);
        ImVec4 frameBgHov   = ImVec4(0.22f, 0.22f, 0.28f, 1.00f);
        ImVec4 frameBgActive= ImVec4(0.26f, 0.26f, 0.34f, 1.00f);
        ImVec4 separator    = ImVec4(0.22f, 0.22f, 0.28f, 1.00f);
        ImVec4 border       = ImVec4(0.22f, 0.22f, 0.28f, 0.60f);

        // Semantic
        ImVec4 success      = ImVec4(0.20f, 0.70f, 0.20f, 1.0f);
        ImVec4 successHov   = ImVec4(0.30f, 0.80f, 0.30f, 1.0f);
        ImVec4 successActive= ImVec4(0.10f, 0.60f, 0.10f, 1.0f);
        ImVec4 danger       = ImVec4(0.80f, 0.20f, 0.20f, 1.0f);
        ImVec4 dangerHov    = ImVec4(0.90f, 0.30f, 0.30f, 1.0f);
        ImVec4 dangerActive = ImVec4(0.70f, 0.10f, 0.10f, 1.0f);
    };

}

#endif
