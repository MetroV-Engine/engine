/*
    Filename: ThemeProvider.hpp
    Description: Initializes ImGui style from the theme tokens and exposes
                 useColors() / useDesign() for components to read live values.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#ifndef PROTO_UI_PROVIDERS_THEMEPROVIDER_HPP
#define PROTO_UI_PROVIDERS_THEMEPROVIDER_HPP

#include "../../theme/Colors.hpp"
#include "../../theme/Design.hpp"

namespace Editor::UI::Providers {

    class ThemeProvider {
    public:
        // Apply the current colors / design to the global ImGui style.
        // Pass the UI scale factor so paddings / sizes scale on hi-DPI screens.
        static void apply(float scale);

        static const Theme::Colors &colors();
        static const Theme::Design &design();
    };

    // Free-function "hooks" mirroring the React API
    inline const Theme::Colors &useColors() { return ThemeProvider::colors(); }
    inline const Theme::Design &useDesign() { return ThemeProvider::design(); }

}

#endif
