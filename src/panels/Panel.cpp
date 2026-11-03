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

// ImGuiDockNodeFlags_NoTabBar is part of the internal docking flags.
#include <imgui_internal.h>

#include <algorithm>
#include <cfloat>

Panel::Panel(std::string name, ImGuiWindowFlags flags) : _name(std::move(name)), _flags(flags) {}

void Panel::drawMoveGrip(float height)
{
    const ImGuiStyle &style = ImGui::GetStyle();
    const float dot = std::max(2.0f, ImGui::GetFontSize() * 0.15f);
    const float gap = dot * 1.8f;
    const ImVec2 size(dot * 2.0f + gap + style.FramePadding.x, height);

    ImGui::InvisibleButton("##MoveGrip", size);
    const bool hovered = ImGui::IsItemHovered();
    const bool held = ImGui::IsItemActive();
    ImGuiWindow *window = ImGui::GetCurrentWindow();
    // Same as dragging a tab: undock the node (or move the floating window).
    if (held && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
        ImGui::StartMouseMovingWindowOrNode(window, window->DockNode, true);
    if (hovered || held)
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
    if (hovered && !held)
        ImGui::SetTooltip("Drag to move this bar");

    // Two columns of three dots, centered in the grip.
    const ImVec2 min = ImGui::GetItemRectMin();
    const ImU32 color = ImGui::GetColorU32(held ? ImGuiCol_Text : hovered ? ImGuiCol_Text : ImGuiCol_TextDisabled);
    const float startX = min.x + (size.x - (dot * 2.0f + gap)) * 0.5f;
    const float startY = min.y + (height - (dot * 3.0f + gap * 2.0f)) * 0.5f;
    for (int column = 0; column < 2; column++)
        for (int row = 0; row < 3; row++) {
            const ImVec2 a(startX + column * (dot + gap), startY + row * (dot + gap));
            window->DrawList->AddRectFilled(a, ImVec2(a.x + dot, a.y + dot), color, dot * 0.5f);
        }

    if (ImGui::BeginPopupContextItem("##MoveGripMenu")) {
        if (ImGui::MenuItem("Undock", nullptr, false, window->DockIsActive))
            ImGui::DockContextQueueUndockWindow(ImGui::GetCurrentContext(), window);
        if (ImGui::MenuItem("Reset Position", nullptr, false, static_cast<bool>(_onResetPosition)))
            _onResetPosition();
        ImGui::EndPopup();
    }
}

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
    // The tab is hidden only while the panel is docked in the editor and alone
    // in its area. Floating, it gets a normal title bar to drag it onto a dock
    // area; docked as a tab next to other panels, everybody needs the tab bar.
    const ImGuiWindow *previous = ImGui::FindWindowByName(_name.c_str());
    // DockIsActive is reset before Begin(); DockNode is kept between frames.
    ImGuiDockNode *node = previous ? previous->DockNode : nullptr;
    const bool dockedAlone = node && node->Windows.Size == 1 && ImGui::DockNodeGetRootNode(node)->IsDockSpace();
    const bool hideTab = _hideTabBar && (dockedAlone || !previous);
    // ImGui copies the flag into the dock node itself (it is saved with the
    // layout and follows the node when it is undocked or merged): clear it as
    // soon as it no longer applies, or other panels would lose their tabs.
    if (_hideTabBar && !hideTab && node && (node->LocalFlags & ImGuiDockNodeFlags_NoTabBar)) {
        node->LocalFlags &= ~ImGuiDockNodeFlags_NoTabBar;
        node->UpdateMergedFlags();
    }
    // Set every frame: ImGui keeps the last window class otherwise.
    if (_hideTabBar) {
        ImGuiWindowClass windowClass;
        windowClass.DockNodeFlagsOverrideSet = hideTab ? static_cast<ImGuiDockNodeFlags>(ImGuiDockNodeFlags_NoTabBar) : 0;
        ImGui::SetNextWindowClass(&windowClass);
    }
    const ImVec2 minimum = minimumSize();
    if (minimum.x > 0.0f || minimum.y > 0.0f)
        ImGui::SetNextWindowSizeConstraints(minimum, ImVec2(FLT_MAX, FLT_MAX));
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
