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

#include <iostream>

ButtonsPanel::ButtonsPanel()
    : Panel("Buttons"),
      _helloButton("Say hello", [] { std::cout << "Hello!" << std::endl; }),
      _cancelButton("Cancel", [] { std::cout << "Cancel clicked" << std::endl; }, Button::Variant::Secondary),
      _quitButton("Quit", [] { std::cout << "Quit clicked" << std::endl; }, Button::Variant::Danger, ImVec2(120, 0))
{
}

void ButtonsPanel::drawContent()
{
    _helloButton.render();
    ImGui::SameLine();
    _cancelButton.render();
    ImGui::SameLine();
    _quitButton.render();
}
