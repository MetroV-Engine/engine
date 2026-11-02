/*
    Created on: 30/03/2026
    Filename: NotificationManager.cpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "NotificationManager.hpp"
#include "theme/ThemeManager.hpp"

#include <imgui.h>

#include <algorithm>
#include <cfloat>
#include <utility>

namespace {

// Seconds a notification stays on screen by default, per level.
constexpr float InfoDuration = 4.0f;
constexpr float WarningDuration = 6.0f;
constexpr float ErrorDuration = 8.0f;
constexpr float FadeInTime = 0.15f;
constexpr float FadeOutTime = 0.4f;
// Notification width, in font heights (so it follows the display scale).
constexpr float WidthInFontSizes = 22.0f;
constexpr float AccentBarWidth = 4.0f;

const ImVec4 &levelColor(NotificationManager::Level level, const Theme &theme)
{
    switch (level) {
    case NotificationManager::Level::Success:
        return theme.colors.success;
    case NotificationManager::Level::Warning:
        return theme.colors.warning;
    case NotificationManager::Level::Error:
        return theme.colors.danger;
    case NotificationManager::Level::Info:
    default:
        return theme.colors.accent;
    }
}

} // namespace

std::uint64_t NotificationManager::notify(Level level, std::string title, std::string message, float duration)
{
    if (duration <= 0.0f)
        duration = level == Level::Error ? ErrorDuration : level == Level::Warning ? WarningDuration : InfoDuration;
    const std::uint64_t id = _nextId++;
    _notifications.push_back({id, level, std::move(title), std::move(message), duration, duration});
    return id;
}

std::uint64_t NotificationManager::info(std::string title, std::string message)
{
    return notify(Level::Info, std::move(title), std::move(message));
}

std::uint64_t NotificationManager::success(std::string title, std::string message)
{
    return notify(Level::Success, std::move(title), std::move(message));
}

std::uint64_t NotificationManager::warning(std::string title, std::string message)
{
    return notify(Level::Warning, std::move(title), std::move(message));
}

std::uint64_t NotificationManager::error(std::string title, std::string message)
{
    return notify(Level::Error, std::move(title), std::move(message));
}

bool NotificationManager::dismiss(std::uint64_t id)
{
    const auto it = std::find_if(
        _notifications.begin(), _notifications.end(), [id](const Notification &n) { return n.id == id; });
    if (it == _notifications.end())
        return false;
    _notifications.erase(it);
    return true;
}

void NotificationManager::draw()
{
    const float dt = ImGui::GetIO().DeltaTime;
    for (Notification &n : _notifications) {
        n.age += dt;
        if (!n.hovered)
            n.remaining -= dt;
    }
    std::erase_if(_notifications, [](const Notification &n) { return n.remaining <= 0.0f; });
    if (_notifications.empty())
        return;

    const Theme &theme = ThemeManager::active();
    const ImGuiStyle &style = ImGui::GetStyle();
    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    const float width = ImGui::GetFontSize() * WidthInFontSizes;
    const float margin = style.WindowPadding.x;
    const float right = viewport->WorkPos.x + viewport->WorkSize.x - margin;
    float bottom = viewport->WorkPos.y + viewport->WorkSize.y - margin;

    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize
        | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav
        | ImGuiWindowFlags_NoMove;

    // Newest at the bottom, older ones stacked above it.
    std::uint64_t clicked = 0;
    const std::size_t visible = std::min(_maxVisible, _notifications.size());
    for (std::size_t slot = 0; slot < visible; slot++) {
        Notification &n = _notifications[_notifications.size() - 1 - slot];
        const float alpha = std::clamp(std::min(n.age / FadeInTime, n.remaining / FadeOutTime), 0.0f, 1.0f);
        const ImVec4 &accent = levelColor(n.level, theme);

        ImGui::SetNextWindowPos(ImVec2(right, bottom), ImGuiCond_Always, ImVec2(1.0f, 1.0f));
        ImGui::SetNextWindowSizeConstraints(ImVec2(width, 0.0f), ImVec2(width, FLT_MAX));
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, theme.colors.surfaceRaised);
        ImGui::PushStyleColor(ImGuiCol_Border, accent);

        // Windows are named by slot, not by notification, so ImGui reuses a
        // handful of windows instead of creating one per notification.
        const std::string name = "##Notification" + std::to_string(slot);
        ImGui::Begin(name.c_str(), nullptr, flags);
        const ImVec2 pos = ImGui::GetWindowPos();
        const ImVec2 size = ImGui::GetWindowSize();
        ImGui::GetWindowDrawList()->AddRectFilled(pos, ImVec2(pos.x + AccentBarWidth, pos.y + size.y),
            ImGui::GetColorU32(accent), style.WindowRounding, ImDrawFlags_RoundCornersLeft);
        ImGui::TextColored(accent, "%s", n.title.c_str());
        if (!n.message.empty())
            ImGui::TextWrapped("%s", n.message.c_str());
        n.hovered = ImGui::IsWindowHovered();
        if (n.hovered && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
            clicked = n.id;
        bottom -= size.y + style.ItemSpacing.y;
        ImGui::End();

        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar();
    }
    if (clicked)
        dismiss(clicked);
}
