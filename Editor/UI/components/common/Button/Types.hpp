#ifndef PROTO_UI_COMMON_BUTTON_TYPES_HPP
#define PROTO_UI_COMMON_BUTTON_TYPES_HPP

#include <imgui.h>

namespace Editor::UI::Components {

    enum class ButtonVariant {
        Default,
        Primary,
        Success,
        Danger,
        Ghost,
    };

    struct ButtonProps {
        ButtonVariant variant = ButtonVariant::Default;
        ImVec2        size    = ImVec2(0, 0); // (0,0) = autosize
        bool          disabled = false;
    };

}

#endif
