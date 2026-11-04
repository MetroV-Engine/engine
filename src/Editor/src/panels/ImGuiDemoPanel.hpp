/*
    Created on: 30/03/2026
    Filename: ImGuiDemoPanel.hpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#pragma once

#include "panels/Panel/Panel.hpp"

// The Dear ImGui demo window as a panel. ShowDemoWindow() creates its own
// window (titled "Dear ImGui Demo"), so draw() is overridden instead of drawContent().
class ImGuiDemoPanel : public Panel
{
public:
    ImGuiDemoPanel() : Panel("Dear ImGui Demo") {}

    void draw() override
    {
        if (!isVisible())
            return;
        bool open = true;
        ImGui::ShowDemoWindow(&open);
        setVisible(open);
    }

protected:
    void drawContent() override {}
};
