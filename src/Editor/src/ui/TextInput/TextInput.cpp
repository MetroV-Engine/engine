/*
    Created on: 30/03/2026
    Filename: TextInput.cpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "TextInput.hpp"

#include <imgui.h>
#include <imgui_stdlib.h>

TextInput::TextInput(std::string label, std::string placeholder, OnChange onChange)
    : _label(std::move(label)), _placeholder(std::move(placeholder)), _onChange(std::move(onChange))
{
}

bool TextInput::render()
{
    ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue;
    if (_password)
        flags |= ImGuiInputTextFlags_Password;

    if (_width != 0.0f)
        ImGui::SetNextItemWidth(_width);
    ImGui::BeginDisabled(!_enabled);

    const std::string previous = _value;
    const bool submitted = ImGui::InputTextWithHint(_label.c_str(), _placeholder.c_str(), &_value, flags);
    const bool changed = _value != previous;

    ImGui::EndDisabled();

    if (changed && _onChange)
        _onChange(_value);
    if (submitted && _onSubmit)
        _onSubmit(_value);
    return changed;
}
