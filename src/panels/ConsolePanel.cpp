/*
    Created on: 30/03/2026
    Filename: ConsolePanel.cpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "ConsolePanel.hpp"
#include "components/Button.hpp"
#include "theme/ThemeManager.hpp"

#include <SDL3/SDL.h>

#include <utility>
#include <vector>

namespace {

const ImVec4 &levelColor(ConsolePanel::Level level, const Theme &theme)
{
    switch (level) {
    case ConsolePanel::Level::Success:
        return theme.colors.success;
    case ConsolePanel::Level::Warning:
        return theme.colors.warning;
    case ConsolePanel::Level::Error:
        return theme.colors.danger;
    case ConsolePanel::Level::Info:
    default:
        return theme.colors.text;
    }
}

} // namespace

ConsolePanel::ConsolePanel() : Panel("Console"), _filter("##ConsoleFilter", "Filter"), _autoScroll("Auto-scroll", true)
{
    _filter.setWidth(200.0f);
}

void ConsolePanel::log(Level level, std::string message)
{
    _entries.push_back({SDL_GetTicks() / 1000.0, level, std::move(message)});
    if (_entries.size() > MaxEntries)
        _entries.pop_front();
}

void ConsolePanel::drawContent()
{
    const Theme &theme = ThemeManager::active();

    if (Button("Clear", {}, Button::Variant::Secondary).render())
        clear();
    ImGui::SameLine();
    _filter.render();
    ImGui::SameLine();
    _autoScroll.render();
    ImGui::Separator();

    // Only the entries matching the filter; recomputed each frame (cheap for
    // MaxEntries lines, and always up to date).
    std::vector<const Entry *> visible;
    const std::string &filter = _filter.getValue();
    for (const Entry &entry : _entries)
        if (filter.empty() || entry.message.find(filter) != std::string::npos)
            visible.push_back(&entry);

    ImGui::BeginChild("##ConsoleLog", ImVec2(0, 0), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar);
    if (visible.empty())
        ImGui::TextDisabled(_entries.empty() ? "No messages." : "No message matches the filter.");

    // Only the visible lines are drawn, so long logs stay fast.
    ImGuiListClipper clipper;
    clipper.Begin(static_cast<int>(visible.size()));
    while (clipper.Step()) {
        for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
            const Entry &entry = *visible[i];
            ImGui::TextDisabled("[%7.2fs]", entry.time);
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Text, levelColor(entry.level, theme));
            ImGui::TextUnformatted(entry.message.c_str());
            ImGui::PopStyleColor();
        }
    }
    // Follows new messages, unless the user scrolled up to read older ones.
    if (_autoScroll.isChecked() && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        ImGui::SetScrollHereY(1.0f);
    ImGui::EndChild();
}
