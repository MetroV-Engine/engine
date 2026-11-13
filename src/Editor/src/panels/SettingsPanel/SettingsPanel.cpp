/*
    Created on: 30/03/2026
    Filename: SettingsPanel.cpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "SettingsPanel.hpp"

#include "core/Logger/Logger.hpp"

#include <string>

SettingsPanel::SettingsPanel(GuiContext &gui, PerformanceOverlay &overlay, ViewportGrid &grid)
    : Panel("Settings"),
      _gui(gui),
      _overlay(overlay),
      _grid(grid),
      _vsyncCheckbox("VSync", true,
          [&gui](bool checked) {
              Logger::instance().info("Settings", std::string("VSync: ") + (checked ? "on" : "off"));
              gui.setVSync(checked);
          }),
      _fullscreenCheckbox("Fullscreen", false,
          [&gui](bool checked) {
              Logger::instance().info("Settings", std::string("Fullscreen: ") + (checked ? "on" : "off"));
              gui.setFullscreen(checked);
          }),
      _resolutionCombo("Resolution", {"1280x720", "1920x1080", "2560x1440"}, 0,
          [this, &gui](int index, const std::string &option) {
              Logger::instance().info("Settings", "Resolution: " + option);
              gui.setWindowSize(_resolutions[index].first, _resolutions[index].second);
          }),
      _qualityRadio("Quality", {"Low", "Medium", "High"}, 1,
          [](int, const std::string &option) { Logger::instance().info("Settings", "Quality: " + option); }),
      _overlayCheckbox("Show performance", overlay.isVisible(),
          [&overlay](bool checked) { overlay.setVisible(checked); }),
      _themeCombo("Theme", gui.themes().getThemeNames(), 0,
          [&gui](int, const std::string &name) { gui.themes().setTheme(name); }),
      _snapCheckbox("Snap to grid", grid.snap, [&grid](bool checked) { grid.snap = checked; }),
      _gridSizeInput("Grid size", grid.size, 1, [&grid](int size) { grid.size = size; })
{
    gui.setVSync(_vsyncCheckbox.isChecked());
    _qualityRadio.setHorizontal(true);
    _gridSizeInput.setRange(ViewportGrid::MinSize, ViewportGrid::MaxSize);
}

void SettingsPanel::drawContent()
{
    // These can also change from the menu bar, toolbar, shortcuts or the
    // window manager: always show the real state.
    _vsyncCheckbox.setChecked(_gui.isVSync());
    _fullscreenCheckbox.setChecked(_gui.isFullscreen());
    _overlayCheckbox.setChecked(_overlay.isVisible());
    const auto &themeNames = _themeCombo.getOptions();
    for (std::size_t i = 0; i < themeNames.size(); i++)
        if (themeNames[i] == _gui.themes().current().name)
            _themeCombo.setSelected(static_cast<int>(i));
    _snapCheckbox.setChecked(_grid.snap);
    _gridSizeInput.setValue(_grid.size);
    _gridSizeInput.setEnabled(_grid.snap);

    _vsyncCheckbox.render();
    _fullscreenCheckbox.render();
    _resolutionCombo.render();
    _qualityRadio.render();
    _overlayCheckbox.render();
    _themeCombo.render();

    ImGui::SeparatorText("Viewport");
    _snapCheckbox.render();
    ImGui::SetItemTooltip("Positions and scales edited with the mouse snap to the grid.");
    _gridSizeInput.render();
    ImGui::SetItemTooltip("World units between two grid lines. Scales snap to tenths.");
}
