/*
    Created on: 30/03/2026
    Filename: InputsPanel.hpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#pragma once

#include "Panel.hpp"
#include "components/NumberInput.hpp"
#include "components/TextInput.hpp"

// Demo panel showing the text and numeric inputs.
class InputsPanel : public Panel
{
public:
    InputsPanel();

protected:
    void drawContent() override;

private:
    TextInput _nameInput;
    TextInput _passwordInput;
    NumberInput<int> _ageInput;
    NumberInput<float> _speedInput;
};
