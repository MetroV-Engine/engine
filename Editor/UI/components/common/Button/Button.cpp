#include "Button.hpp"

#include "../../../providers/Theme/ThemeProvider.hpp"

namespace Editor::UI::Components {

    static void pushVariantColors(ButtonVariant v)
    {
        const auto &c = Providers::useColors();
        switch (v) {
            case ButtonVariant::Success:
                ImGui::PushStyleColor(ImGuiCol_Button,        c.success);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, c.successHov);
                ImGui::PushStyleColor(ImGuiCol_ButtonActive,  c.successActive);
                break;
            case ButtonVariant::Danger:
                ImGui::PushStyleColor(ImGuiCol_Button,        c.danger);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, c.dangerHov);
                ImGui::PushStyleColor(ImGuiCol_ButtonActive,  c.dangerActive);
                break;
            case ButtonVariant::Primary:
            case ButtonVariant::Ghost:
            case ButtonVariant::Default:
            default:
                ImGui::PushStyleColor(ImGuiCol_Button,        c.button);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, c.buttonHov);
                ImGui::PushStyleColor(ImGuiCol_ButtonActive,  c.buttonActive);
                break;
        }
    }

    bool Button::render(const std::string &label, const ButtonProps &props)
    {
        pushVariantColors(props.variant);
        if (props.disabled) ImGui::BeginDisabled();
        const bool clicked = ImGui::Button(label.c_str(), props.size);
        if (props.disabled) ImGui::EndDisabled();
        ImGui::PopStyleColor(3);
        return clicked;
    }

}
