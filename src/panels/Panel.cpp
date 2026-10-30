/*
    Created on: 30/03/2026
    Filename: Panel.cpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "Panel.hpp"

Panel::Panel(std::string name, ImGuiWindowFlags flags) : _name(std::move(name)), _flags(flags) {}

void Panel::draw()
{
    if (!_visible)
        return;

    if (_initialSize.x > 0.0f || _initialSize.y > 0.0f)
        ImGui::SetNextWindowSize(_initialSize, ImGuiCond_FirstUseEver);
    // The close button (X) sets _visible to false; the View menu reopens it.
    if (ImGui::Begin(_name.c_str(), _closable ? &_visible : nullptr, _flags))
        drawContent();
    ImGui::End();
}
