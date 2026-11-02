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
    if (_focusRequested) {
        ImGui::SetNextWindowFocus();
        _focusRequested = false;
    }
    if (_windowPadding)
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, *_windowPadding);
    // Panels do not grab focus when they first appear: otherwise, at startup,
    // the last panel drawn in each dock area would select its tab and override
    // the tabs saved in imgui.ini. They get focus when shown explicitly.
    // The close button (X) sets _visible to false; the View menu reopens it.
    const bool open = ImGui::Begin(
        _name.c_str(), _closable ? &_visible : nullptr, _flags | ImGuiWindowFlags_NoFocusOnAppearing);
    // The padding applies to the window itself, not to the widgets inside.
    if (_windowPadding)
        ImGui::PopStyleVar();
    if (open)
        drawContent();
    ImGui::End();
}
