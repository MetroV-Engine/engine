/*
    Created on: 30/03/2026
    Filename: StatusBar.cpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "StatusBar.hpp"

#include <imgui.h>
// BeginViewportSideBar() reserves space at the bottom of the viewport, like
// the menu bar does at the top (no public equivalent yet).
#include <imgui_internal.h>

void StatusBar::draw() const
{
    if (!_visible)
        return;

    const float height = ImGui::GetFrameHeight();
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings
        | ImGuiWindowFlags_MenuBar;
    // Drawn as a menu bar inside the side bar: same look and height as the main menu bar.
    if (ImGui::BeginViewportSideBar("##StatusBar", ImGui::GetMainViewport(), ImGuiDir_Down, height, flags)) {
        if (ImGui::BeginMenuBar()) {
            ImGui::TextUnformatted(_message.c_str());

            if (!_items.empty()) {
                // ImGui cannot measure widgets before drawing them: the items
                // are right-aligned with the width measured on the last frame.
                const ImGuiID widthId = ImGui::GetID("##StatusBarItemsWidth");
                ImGuiStorage *storage = ImGui::GetStateStorage();
                const float itemsWidth = storage->GetFloat(widthId, 0.0f);
                const float spacing = ImGui::GetStyle().ItemSpacing.x;
                ImGui::SameLine();
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - itemsWidth);

                const float start = ImGui::GetCursorPosX();
                for (std::size_t i = 0; i < _items.size(); i++) {
                    if (i > 0) {
                        ImGui::SameLine(0.0f, spacing);
                        ImGui::TextDisabled("|");
                        ImGui::SameLine(0.0f, spacing);
                    }
                    _items[i]();
                }
                ImGui::SameLine(0.0f, 0.0f);
                storage->SetFloat(widthId, ImGui::GetCursorPosX() - start);
            }
            ImGui::EndMenuBar();
        }
    }
    ImGui::End();
}
