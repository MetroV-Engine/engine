/*
    Created on: 30/03/2026
    Filename: ButtonsPanel.cpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "ButtonsPanel.hpp"

#include <utility>

ButtonsPanel::ButtonsPanel(NotificationManager &notifications, DialogManager &dialogs, std::function<void()> onQuit)
    : Panel("Buttons"),
      _helloButton("Say hello", [&notifications] { notifications.success("Hello!", "Everything works."); }),
      _cancelButton("Cancel", [&notifications] { notifications.info("Cancelled"); }, Button::Variant::Secondary),
      _quitButton("Quit", std::move(onQuit), Button::Variant::Danger, ImVec2(120, 0)),
      _warningButton("Warning", [&notifications] {
          notifications.warning("Low disk space", "Only 2 GB left on the project drive.");
      }, Button::Variant::Secondary),
      _errorButton("Error", [&notifications] {
          notifications.error("Build failed", "main.cpp:42: expected ';' before '}' token");
      }, Button::Variant::Secondary),
      _alertButton("Alert", [&dialogs] {
          dialogs.alert("About this demo", "Dialogs block the editor until they are answered. Press Enter or click OK.");
      }, Button::Variant::Secondary)
{
}

void ButtonsPanel::drawContent()
{
    _helloButton.render();
    ImGui::SameLine();
    _cancelButton.render();
    ImGui::SameLine();
    _quitButton.render();

    ImGui::TextDisabled("Feedback");
    _warningButton.render();
    ImGui::SameLine();
    _errorButton.render();
    ImGui::SameLine();
    _alertButton.render();
}
