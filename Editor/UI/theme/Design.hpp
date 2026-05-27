/*
    Filename: Design.hpp
    Description: Design tokens — spacing, radius, sizes. Accessed via free functions.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#ifndef PROTO_UI_THEME_DESIGN_HPP
#define PROTO_UI_THEME_DESIGN_HPP

namespace Editor::UI::Theme {

    struct Design {
        float windowRounding   = 4.0f;
        float frameRounding    = 3.0f;
        float grabRounding     = 3.0f;
        float tabRounding      = 4.0f;
        float scrollbarRounding= 6.0f;
        float windowBorderSize = 1.0f;
        float frameBorderSize  = 0.0f;

        float windowPadX = 8.0f;
        float windowPadY = 8.0f;
        float framePadX  = 6.0f;
        float framePadY  = 4.0f;
        float itemSpaceX = 8.0f;
        float itemSpaceY = 5.0f;

        // Layout
        float minToolbarHeight   = 50.0f;
        float minProjectWidth    = 250.0f;
        float minInspectorWidth  = 250.0f;
        float maxSidePanelWidth  = 400.0f;
        float toolbarHeightRatio = 0.05f;
        float sidePanelRatio     = 0.25f;

        // Buttons
        float playButtonWidth  = 90.0f;
        float playButtonHeight = 40.0f;
        float buttonPadding    = 10.0f;
        float minPlayButtonW   = 70.0f;
        float minPlayButtonH   = 30.0f;
    };

}

#endif
