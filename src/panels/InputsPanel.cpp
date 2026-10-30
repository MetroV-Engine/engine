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

#include <iostream>

InputsPanel::InputsPanel()
    : Panel("Inputs"),
      _nameInput("Name", "Enter your name...",
          [](const std::string &value) { std::cout << "Name: " << value << std::endl; }),
      _passwordInput("Password", "Secret"),
      _ageInput("Age", 18, 1, [](int value) { std::cout << "Age: " << value << std::endl; }),
      _speedInput("Speed", 1.0f, 0.1f, [](float value) { std::cout << "Speed: " << value << std::endl; })
{
    _nameInput.setOnSubmit([](const std::string &value) { std::cout << "Name submitted: " << value << std::endl; });
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
