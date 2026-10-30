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
#include "navigation/ActionRegistry.hpp"
#include "navigation/MenuBar.hpp"
#include "navigation/Toolbar.hpp"
#include "panels/ButtonsPanel.hpp"
#include "panels/ImGuiDemoPanel.hpp"
#include "panels/InputsPanel.hpp"
#include "panels/SettingsPanel.hpp"

#include <imgui.h>

#include <exception>
#include <iostream>
#include <string>

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

        panels.registerPanel("About", [] {
            ImGui::TextUnformatted("MetroV Engine - GUI");
            ImGui::TextDisabled("Built with SDL3 and Dear ImGui %s", IMGUI_VERSION);
        }, ImGuiWindowFlags_AlwaysAutoResize).setVisible(false);

        // Every command is defined once here, then placed in the menu bar,
        // the toolbar and/or bound to a shortcut.
        ActionRegistry actions;
        actions.add({.id = "file.quit", .label = "Quit", .callback = [&gui] { gui.stop(); },
            .shortcut = ImGuiMod_Ctrl | ImGuiKey_Q});
        actions.add({.id = "view.fullscreen", .label = "Fullscreen",
            .callback = [&gui] { gui.setFullscreen(!gui.isFullscreen()); }, .shortcut = ImGuiKey_F11,
            .tooltip = "Toggle fullscreen", .isChecked = [&gui] { return gui.isFullscreen(); }});
        actions.add({.id = "view.performance", .label = "Performance",
            .callback = [&performanceOverlay] { performanceOverlay.setVisible(!performanceOverlay.isVisible()); },
            .shortcut = ImGuiKey_F3, .tooltip = "Show FPS and frame time",
            .isChecked = [&performanceOverlay] { return performanceOverlay.isVisible(); }});
        for (const auto &panel : panels.getPanels()) {
            Panel *p = panel.get();
            actions.add({.id = "view.panel." + p->getName(), .label = p->getName(),
                .callback = [p] { p->setVisible(!p->isVisible()); }, .tooltip = "Show/hide the " + p->getName() + " panel",
                .isChecked = [p] { return p->isVisible(); }});
        }

        MenuBar menuBar(actions);
        menuBar.addAction("File", "file.quit");
        for (const char *name : {"Buttons", "Inputs", "Settings"})
            menuBar.addAction("View/Panels", std::string("view.panel.") + name);
        menuBar.addSeparator("View");
        menuBar.addAction("View", "view.fullscreen");
        menuBar.addAction("View", "view.performance");
        menuBar.addAction("Help", "view.panel.Dear ImGui Demo");
        menuBar.addAction("Help", "view.panel.About");

        Toolbar toolbar(actions);
        toolbar.addAction("view.panel.Buttons");
        toolbar.addAction("view.panel.Inputs");
        toolbar.addAction("view.panel.Settings");
        toolbar.addSeparator();
        toolbar.addAction("view.fullscreen");
        toolbar.addAction("view.performance");

        gui.run([&] {
            actions.processShortcuts();
            menuBar.draw();
            toolbar.draw();
            panels.drawAll();
            performanceOverlay.render();
        });
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}
