#include "ThemeProvider.hpp"

#include <imgui.h>

namespace Editor::UI::Providers {

    static Theme::Colors g_colors;
    static Theme::Design g_design;

    const Theme::Colors &ThemeProvider::colors() { return g_colors; }
    const Theme::Design &ThemeProvider::design() { return g_design; }

    void ThemeProvider::apply(float scale)
    {
        ImGui::StyleColorsDark();
        ImGuiStyle &style = ImGui::GetStyle();

        style.WindowRounding    = g_design.windowRounding;
        style.FrameRounding     = g_design.frameRounding;
        style.GrabRounding      = g_design.grabRounding;
        style.TabRounding       = g_design.tabRounding;
        style.ScrollbarRounding = g_design.scrollbarRounding;
        style.WindowBorderSize  = g_design.windowBorderSize;
        style.FrameBorderSize   = g_design.frameBorderSize;
        style.WindowPadding     = ImVec2(g_design.windowPadX, g_design.windowPadY);
        style.FramePadding      = ImVec2(g_design.framePadX, g_design.framePadY);
        style.ItemSpacing       = ImVec2(g_design.itemSpaceX, g_design.itemSpaceY);
        style.ScaleAllSizes(scale);

        auto &c = style.Colors;
        c[ImGuiCol_WindowBg]       = g_colors.windowBg;
        c[ImGuiCol_TitleBg]        = g_colors.titleBg;
        c[ImGuiCol_TitleBgActive]  = g_colors.titleBgActive;
        c[ImGuiCol_Tab]            = g_colors.tab;
        c[ImGuiCol_TabSelected]    = g_colors.tabSelected;
        c[ImGuiCol_TabHovered]     = g_colors.tabHovered;
        c[ImGuiCol_Header]         = g_colors.header;
        c[ImGuiCol_HeaderHovered]  = g_colors.headerHov;
        c[ImGuiCol_HeaderActive]   = g_colors.headerActive;
        c[ImGuiCol_Button]         = g_colors.button;
        c[ImGuiCol_ButtonHovered]  = g_colors.buttonHov;
        c[ImGuiCol_ButtonActive]   = g_colors.buttonActive;
        c[ImGuiCol_FrameBg]        = g_colors.frameBg;
        c[ImGuiCol_FrameBgHovered] = g_colors.frameBgHov;
        c[ImGuiCol_FrameBgActive]  = g_colors.frameBgActive;
        c[ImGuiCol_Separator]      = g_colors.separator;
        c[ImGuiCol_Border]         = g_colors.border;
    }

}
