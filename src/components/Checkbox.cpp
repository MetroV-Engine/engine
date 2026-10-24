/*
    Created on: 30/03/2026
    Filename: Checkbox.cpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "Checkbox.hpp"

#include <imgui.h>

Checkbox::Checkbox(std::string label, bool checked, OnChange onChange)
    : _label(std::move(label)), _checked(checked), _onChange(std::move(onChange))
{
}

bool Checkbox::render()
{
    ImGui::BeginDisabled(!_enabled);
    const bool changed = ImGui::Checkbox(_label.c_str(), &_checked);
    ImGui::EndDisabled();

    if (changed && _onChange)
        _onChange(_checked);
    return changed;
}
