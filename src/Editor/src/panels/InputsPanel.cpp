/*
    Created on: 30/03/2026
    Filename: InputsPanel.cpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "InputsPanel.hpp"

#include "log/Logger.hpp"

#include <string>

InputsPanel::InputsPanel()
    : Panel("Inputs"),
      _nameInput("Name", "Enter your name...",
          [](const std::string &value) { Logger::instance().info("Demo", "Name: " + value); }),
      _passwordInput("Password", "Secret"),
      _ageInput("Age", 18, 1, [](int value) { Logger::instance().info("Demo", "Age: " + std::to_string(value)); }),
      _speedInput("Speed", 1.0f, 0.1f, [](float value) { Logger::instance().info("Demo", "Speed: " + std::to_string(value)); })
{
    _nameInput.setOnSubmit([](const std::string &value) { Logger::instance().info("Demo", "Name submitted: " + value); });
    _passwordInput.setPassword(true);
    _ageInput.setRange(0, 120);
    _speedInput.setRange(0.0f, 10.0f);
}

void InputsPanel::drawContent()
{
    _nameInput.render();
    _passwordInput.render();
    _ageInput.render();
    _speedInput.render();
}
