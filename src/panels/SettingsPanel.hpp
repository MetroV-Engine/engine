/*
    Created on: 30/03/2026
    Filename: SettingsPanel.hpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#pragma once

#include "GuiContext.hpp"
#include "Panel.hpp"
#include "components/Checkbox.hpp"
#include "components/ComboBox.hpp"
#include "components/PerformanceOverlay.hpp"
#include "components/RadioGroup.hpp"

#include <utility>
#include <vector>

// Window and display settings: VSync, fullscreen, resolution, quality and
// the performance overlay.
class SettingsPanel : public Panel
{
public:
    SettingsPanel(GuiContext &gui, PerformanceOverlay &overlay);

protected:
    void drawContent() override;

private:
    GuiContext &_gui;
    PerformanceOverlay &_overlay;
    const std::vector<std::pair<int, int>> _resolutions = {{1280, 720}, {1920, 1080}, {2560, 1440}};
    Checkbox _vsyncCheckbox;
    Checkbox _fullscreenCheckbox;
    ComboBox _resolutionCombo;
    RadioGroup _qualityRadio;
    Checkbox _overlayCheckbox;
};
