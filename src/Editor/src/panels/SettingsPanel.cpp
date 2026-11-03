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

#include "log/Logger.hpp"

#include <string>

SettingsPanel::SettingsPanel(GuiContext &gui, PerformanceOverlay &overlay)
    : Panel("Settings"),
      _gui(gui),
      _overlay(overlay),
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
          [&gui](int, const std::string &name) { gui.themes().setTheme(name); })
{
    gui.setVSync(_vsyncCheckbox.isChecked());
    _qualityRadio.setHorizontal(true);
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

    _vsyncCheckbox.render();
    _fullscreenCheckbox.render();
    _resolutionCombo.render();
    _qualityRadio.render();
    _overlayCheckbox.render();
    _themeCombo.render();
}
