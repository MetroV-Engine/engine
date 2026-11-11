/*
    Created on: 30/03/2026
    Filename: PlayControlsPanel.cpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "PlayControlsPanel.hpp"
#include "ui/ThemeManager/ThemeManager.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace {

constexpr float TimeScales[] = {0.25f, 0.5f, 1.0f, 2.0f, 4.0f};
constexpr const char *ControlActions[] = {"game.play", "game.pause", "game.stop", "game.step"};

// Between the buttons, the status and the speed.
float groupGap()
{
    return ImGui::GetStyle().ItemSpacing.x * 3.0f;
}

float dotRadius()
{
    return ImGui::GetFontSize() * 0.3f;
}

float speedSelectorWidth()
{
    return ImGui::GetFontSize() * 4.5f;
}

ImVec4 withAlpha(ImVec4 color, float alpha)
{
    color.w *= alpha;
    return color;
}

} // namespace

PlayControlsPanel::PlayControlsPanel(const ActionRegistry &actions, PlayController &play)
    : Panel("Game Controls", ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse), _actions(actions), _play(play)
{
    setHideTabBar(true);
}

float PlayControlsPanel::buttonWidth(const char *label, float height)
{
    // Left margin, icon, gap, label, right margin.
    return height * 0.55f + height * 0.42f + ImGui::GetStyle().ItemInnerSpacing.x + ImGui::CalcTextSize(label).x + height * 0.45f;
}

void PlayControlsPanel::drawIcon(ImDrawList *drawList, Icon icon, ImVec2 center, float size, ImU32 color)
{
    const float h = size * 0.5f;
    switch (icon) {
    case Icon::Play:
        drawList->AddTriangleFilled(ImVec2(center.x - h * 0.7f, center.y - h), ImVec2(center.x - h * 0.7f, center.y + h),
            ImVec2(center.x + h, center.y), color);
        break;
    case Icon::Pause: {
        const float bar = size * 0.28f;
        drawList->AddRectFilled(ImVec2(center.x - h * 0.8f, center.y - h), ImVec2(center.x - h * 0.8f + bar, center.y + h), color, 1.0f);
        drawList->AddRectFilled(ImVec2(center.x + h * 0.8f - bar, center.y - h), ImVec2(center.x + h * 0.8f, center.y + h), color, 1.0f);
        break;
    }
    case Icon::Stop:
        drawList->AddRectFilled(ImVec2(center.x - h * 0.85f, center.y - h * 0.85f), ImVec2(center.x + h * 0.85f, center.y + h * 0.85f), color, 2.0f);
        break;
    case Icon::Step:
        // Play triangle followed by a bar: "advance one frame".
        drawList->AddTriangleFilled(ImVec2(center.x - h, center.y - h), ImVec2(center.x - h, center.y + h),
            ImVec2(center.x + h * 0.4f, center.y), color);
        drawList->AddRectFilled(ImVec2(center.x + h * 0.5f, center.y - h), ImVec2(center.x + h, center.y + h), color, 1.0f);
        break;
    }
}

bool PlayControlsPanel::controlButton(const char *actionId, Icon icon, const ImVec4 &color, float height, bool label) const
{
    const Action *action = _actions.get(actionId);
    if (!action)
        return false;
    const Theme &theme = ThemeManager::active();
    const ImGuiStyle &style = ImGui::GetStyle();
    const bool enabled = action->enabled();
    const bool active = action->checked();

    const float iconSize = height * 0.42f;
    const ImVec2 labelSize = ImGui::CalcTextSize(action->label.c_str());
    const ImVec2 size(label ? buttonWidth(action->label.c_str(), height) : height, height);

    ImGui::PushID(actionId);
    ImGui::BeginDisabled(!enabled);
    const bool clicked = ImGui::InvisibleButton("##Control", size);
    ImGui::EndDisabled();
    const bool hovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled);
    const bool held = ImGui::IsItemActive();
    ImGui::PopID();

    const ImVec2 min = ImGui::GetItemRectMin();
    const ImVec2 max = ImGui::GetItemRectMax();
    const InteractiveColors shades = theme.interactive(color);
    // The button of the current state stays bright (it is the state, even if
    // pressing it again does nothing); other unavailable buttons fade out.
    const bool faded = !enabled && !active;
    ImVec4 fill = faded ? withAlpha(color, 0.25f) : held ? shades.active : hovered && enabled ? shades.hovered : shades.base;
    ImDrawList *drawList = ImGui::GetWindowDrawList();
    const float rounding = height * 0.3f;
    drawList->AddRectFilled(min, max, ImGui::GetColorU32(fill), rounding);
    // The button of the current state glows.
    if (active)
        drawList->AddRect(ImVec2(min.x - 2, min.y - 2), ImVec2(max.x + 2, max.y + 2), ImGui::GetColorU32(withAlpha(color, 0.7f)),
            rounding + 2, 0, 2.0f);

    const ImU32 foreground = ImGui::GetColorU32(withAlpha(theme.colors.textOnAccent, faded ? 0.45f : 1.0f));
    const ImVec2 iconCenter(label ? min.x + height * 0.55f + iconSize * 0.5f - height * 0.1f : (min.x + max.x) * 0.5f,
        (min.y + max.y) * 0.5f);
    drawIcon(drawList, icon, iconCenter, iconSize, foreground);
    if (label)
        drawList->AddText(ImVec2(iconCenter.x + iconSize * 0.5f + style.ItemInnerSpacing.x + height * 0.1f,
                              (min.y + max.y - labelSize.y) * 0.5f),
            foreground, action->label.c_str());

    if (hovered) {
        std::string tooltip = label ? action->tooltip : action->label + ": " + action->tooltip;
        if (const std::string shortcut = _actions.shortcutLabel(*action); !shortcut.empty())
            tooltip += "  -  " + shortcut;
        ImGui::SetItemTooltip("%s", tooltip.c_str());
    }
    return clicked && enabled;
}

std::string PlayControlsPanel::statusText(PlayState state, double time, bool captions)
{
    if (captions && state == PlayState::Stopped)
        return "Editing";
    const int minutes = static_cast<int>(time / 60.0);
    char text[64];
    if (captions)
        std::snprintf(text, sizeof(text), "%s  %02d:%05.2f", PlayController::stateName(state), minutes, time - minutes * 60.0);
    else
        std::snprintf(text, sizeof(text), "%02d:%05.2f", minutes, time - minutes * 60.0);
    return text;
}

float PlayControlsPanel::statusWidth(bool captions)
{
    // Measured with the timer at zero: digits are equally wide.
    float width = 0.0f;
    for (const PlayState state : {PlayState::Stopped, PlayState::Playing, PlayState::Paused})
        width = std::max(width, ImGui::CalcTextSize(statusText(state, 0.0, captions).c_str()).x);
    return width;
}

float PlayControlsPanel::rowWidth(const RowParts &parts, float height) const
{
    // The same items and gaps as drawContent() draws.
    const float spacing = ImGui::GetStyle().ItemSpacing.x;
    float width = 0.0f;
    for (const char *id : ControlActions)
        if (const Action *action = _actions.get(id))
            width += (parts.labels ? buttonWidth(action->label.c_str(), height) : height) + spacing;
    width += groupGap() - spacing;
    if (parts.status)
        width += dotRadius() * 2.0f + spacing + statusWidth(parts.captions) + groupGap();
    if (parts.captions)
        width += ImGui::CalcTextSize("Speed").x + spacing;
    return width + speedSelectorWidth();
}

ImVec2 PlayControlsPanel::minimumSize() const
{
    const ImGuiStyle &style = ImGui::GetStyle();
    const float frame = ImGui::GetFrameHeight();
    return ImVec2(frame * 20.0f, frame + frame * 1.6f + style.WindowPadding.y * 2.0f);
}

void PlayControlsPanel::drawContent()
{
    const Theme &theme = ThemeManager::active();
    const ImGuiStyle &style = ImGui::GetStyle();
    const float height = std::max(ImGui::GetFrameHeight() * 1.4f, std::min(ImGui::GetContentRegionAvail().y, ImGui::GetFrameHeight() * 1.6f));

    // Status: a colored dot, the state and the timer.
    const PlayState state = _play.getState();
    const ImVec4 &stateColor = state == PlayState::Playing ? theme.colors.success
        : state == PlayState::Paused                       ? theme.colors.warning
                                                           : theme.colors.textMuted;
    const float rowTop = std::max(ImGui::GetCursorPosY(), (ImGui::GetWindowHeight() - height) * 0.5f);

    // Handle on the left, to move the bar (it has no tab to drag).
    ImGui::SetCursorPosY(rowTop);
    drawMoveGrip(height);
    ImGui::SameLine();

    // What fits in the room left, centered: buttons, then status and speed.
    const float available = ImGui::GetContentRegionAvail().x - style.ItemSpacing.x;
    RowParts parts;
    if (rowWidth(parts, height) > available)
        parts.labels = false;
    if (rowWidth(parts, height) > available)
        parts.captions = false;
    if (rowWidth(parts, height) > available)
        parts.status = false;
    ImGui::SetCursorPosX(
        std::max(ImGui::GetCursorPosX() + style.ItemSpacing.x, (ImGui::GetWindowWidth() - rowWidth(parts, height)) * 0.5f));
    ImGui::SetCursorPosY(rowTop);

    if (controlButton("game.play", Icon::Play, theme.colors.success, height, parts.labels))
        _actions.execute("game.play");
    ImGui::SameLine();
    if (controlButton("game.pause", Icon::Pause, theme.colors.warning, height, parts.labels))
        _actions.execute("game.pause");
    ImGui::SameLine();
    if (controlButton("game.stop", Icon::Stop, theme.colors.danger, height, parts.labels))
        _actions.execute("game.stop");
    ImGui::SameLine();
    if (controlButton("game.step", Icon::Step, theme.colors.secondary, height, parts.labels))
        _actions.execute("game.step");

    // The rest of the row is vertically centered on the buttons: each item
    // starts a frame-height line in the middle of the button height.
    ImGui::SameLine(0.0f, groupGap());
    const float rowY = ImGui::GetCursorPosY();
    const float lineY = rowY + (height - ImGui::GetFrameHeight()) * 0.5f;
    const auto centerNext = [lineY] {
        ImGui::SetCursorPosY(lineY);
        ImGui::AlignTextToFramePadding();
    };

    if (parts.status) {
        // State dot (pulsing while the game runs) and timer.
        centerNext();
        const ImVec2 dot = ImGui::GetCursorScreenPos();
        const float radius = dotRadius();
        const float pulse = state == PlayState::Playing ? 0.6f + 0.4f * std::sin(static_cast<float>(ImGui::GetTime()) * 5.0f) : 1.0f;
        ImGui::GetWindowDrawList()->AddCircleFilled(ImVec2(dot.x + radius, dot.y + ImGui::GetFrameHeight() * 0.5f), radius,
            ImGui::GetColorU32(withAlpha(stateColor, pulse)));
        ImGui::Dummy(ImVec2(radius * 2.0f, ImGui::GetFrameHeight()));
        ImGui::SameLine();
        centerNext();
        const float statusX = ImGui::GetCursorPosX();
        ImGui::TextColored(stateColor, "%s", statusText(state, _play.getPlayTime(), parts.captions).c_str());
        // The speed after the room of the widest status.
        ImGui::SameLine(statusX + statusWidth(parts.captions) + groupGap());
    }

    // Speed selector.
    if (parts.captions) {
        centerNext();
        ImGui::TextDisabled("Speed");
        ImGui::SameLine();
    }
    ImGui::SetCursorPosY(lineY);
    char preview[16];
    std::snprintf(preview, sizeof(preview), "x%.2g", _play.getTimeScale());
    ImGui::SetNextItemWidth(speedSelectorWidth());
    const bool speedOpen = ImGui::BeginCombo("##Speed", preview);
    ImGui::SetItemTooltip("Speed of the game");
    if (speedOpen) {
        for (const float scale : TimeScales) {
            char label[16];
            std::snprintf(label, sizeof(label), "x%.2g", scale);
            if (ImGui::Selectable(label, std::abs(_play.getTimeScale() - scale) < 1e-6))
                _play.setTimeScale(scale);
        }
        ImGui::EndCombo();
    }
}
