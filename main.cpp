/*
    Created on: 30/03/2026
    Filename: main.cpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "GuiContext.hpp"
#include "PanelManager.hpp"
#include "components/PerformanceOverlay.hpp"
#include "panels/ButtonsPanel.hpp"
#include "panels/ImGuiDemoPanel.hpp"
#include "panels/InputsPanel.hpp"
#include "panels/SettingsPanel.hpp"

#include <imgui.h>

#include <exception>
#include <iostream>

int main()
{
    try {
        GuiContext gui("GUI", 1280, 720);

        PerformanceOverlay performanceOverlay(gui.timer());

        PanelManager panels;
        panels.registerPanel<ButtonsPanel>();
        panels.registerPanel<InputsPanel>();
        panels.registerPanel<SettingsPanel>(gui, performanceOverlay);
        panels.registerPanel<ImGuiDemoPanel>().setVisible(false);

        panels.setMenuBarCallback([&gui] {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Quit", "Alt+F4"))
                    gui.stop();
                ImGui::EndMenu();
            }
        });

        gui.run([&] {
            panels.drawAll();
            performanceOverlay.render();
        });
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}
