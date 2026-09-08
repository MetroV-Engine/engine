/*
    Filename: UseWindowLayout.hpp
    Description: Computes the editor's panel rectangles based on the current
                 window size + design tokens. Returned values are constant for
                 a given frame and can be cached by the caller.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚════╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#ifndef PROTO_UI_HOOKS_USE_WINDOW_LAYOUT_HPP
#define PROTO_UI_HOOKS_USE_WINDOW_LAYOUT_HPP

namespace Editor::UI::Hooks {

    struct WindowLayout {
        float toolbarHeight    = 0.f;
        float projectWidth     = 0.f;
        float inspectorWidth   = 0.f;
        float contentHeight    = 0.f;  // height of the row under the toolbar
        float viewportWidth    = 0.f;  // width remaining after Project (Inspector docks inside)
        float winWidth         = 0.f;
        float winHeight        = 0.f;
    };

    // Compute the layout for the current frame. Reads ScaleProvider for window
    // size and ThemeProvider for design tokens.
    WindowLayout useWindowLayout();

}

#endif
