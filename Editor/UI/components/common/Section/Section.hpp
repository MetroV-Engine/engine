/*
    Filename: Section.hpp
    Description: Reusable collapsible header. Wraps ImGui::CollapsingHeader with
                 an optional right-aligned trailing button (e.g. "Remove").

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚════╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#ifndef PROTO_UI_COMMON_SECTION_HPP
#define PROTO_UI_COMMON_SECTION_HPP

#include <functional>
#include <string>

namespace Editor::UI::Components {

    class Section {
    public:
        // Returns true if the section is expanded; only render the body when true.
        static bool begin(const std::string &title);

        // Optional convenience: a section that takes a body lambda + a trailing
        // "x" button. Returns whether the trailing button was clicked.
        static bool renderWithRemove(const std::string &title,
                                     const std::function<void()> &body);
    };

}

#endif
