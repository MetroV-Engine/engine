/*
    Created on: 30/03/2026
    Filename: CommandPalette.cpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "CommandPalette.hpp"

#include <imgui_stdlib.h>

#include <algorithm>
#include <cctype>
#include <cfloat>

namespace {

constexpr const char *PopupId = "##CommandPalette";

// Up/Down arrive through the history callback while the text field is focused.
int queryCallback(ImGuiInputTextCallbackData *data)
{
    int &move = *static_cast<int *>(data->UserData);
    if (data->EventFlag == ImGuiInputTextFlags_CallbackHistory)
        move += data->EventKey == ImGuiKey_UpArrow ? -1 : 1;
    return 0;
}

} // namespace

int CommandPalette::fuzzyScore(const std::string &query, const std::string &text)
{
    if (query.empty())
        return 0;
    std::string lowerText = text;
    std::ranges::transform(lowerText, lowerText.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    int score = 0;
    std::size_t position = 0;
    std::size_t previous = std::string::npos;
    for (const char raw : query) {
        const char c = static_cast<char>(std::tolower(static_cast<unsigned char>(raw)));
        if (c == ' ')
            continue;
        position = lowerText.find(c, position);
        if (position == std::string::npos)
            return -1;
        score += 1;
        if (position == 0 || lowerText[position - 1] == ' ' || lowerText[position - 1] == ':')
            score += 8; // Start of a word
        if (previous != std::string::npos && position == previous + 1)
            score += 5; // Consecutive letters
        else if (previous != std::string::npos)
            score -= static_cast<int>(std::min<std::size_t>(position - previous - 1, 5)); // Gap
        previous = position;
        position++;
    }
    // The whole query as a substring is the best kind of match.
    std::string lowerQuery = query;
    std::ranges::transform(lowerQuery, lowerQuery.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (lowerText.find(lowerQuery) != std::string::npos)
        score += 20;
    return score;
}

std::string CommandPalette::displayName(const Action &action)
{
    return ActionRegistry::categoryOf(action.id) + ": " + action.label;
}

std::vector<const Action *> CommandPalette::matches() const
{
    std::vector<std::pair<int, const Action *>> scored;
    for (const auto &action : _actions.getActions()) {
        if (!action->callback)
            continue;
        const int score = fuzzyScore(_query, displayName(*action));
        if (score < 0)
            continue;
        int bonus = 0;
        if (_query.empty()) {
            // Without a query: recent commands first, in order of use.
            const auto recent = std::ranges::find(_recent, action->id);
            if (recent != _recent.end())
                bonus = 1000 - static_cast<int>(recent - _recent.begin());
        }
        scored.emplace_back(score + bonus, action.get());
    }
    std::ranges::stable_sort(scored, [](const auto &a, const auto &b) { return a.first > b.first; });

    std::vector<const Action *> result;
    for (const auto &[score, action] : scored)
        result.push_back(action);
    return result;
}

void CommandPalette::run(const std::string &id)
{
    std::erase(_recent, id);
    _recent.insert(_recent.begin(), id);
    if (_recent.size() > MaxRecent)
        _recent.pop_back();
    _actions.execute(id);
}

void CommandPalette::draw()
{
    // Closing must happen inside the popup (CloseCurrentPopup), below.
    bool closeRequested = false;
    if (_toggleRequested) {
        _toggleRequested = false;
        if (_open) {
            closeRequested = true;
        } else {
            ImGui::OpenPopup(PopupId);
            _query.clear();
            _selected = 0;
        }
    }

    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    const float fontSize = ImGui::GetFontSize();
    ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + viewport->WorkSize.x * 0.5f, viewport->WorkPos.y + fontSize * 2.0f),
        ImGuiCond_Always, ImVec2(0.5f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(std::min(fontSize * 38.0f, viewport->WorkSize.x - fontSize * 2.0f), 0.0f));
    if (!ImGui::BeginPopup(PopupId)) {
        _open = false;
        return;
    }
    if (!_open)
        ImGui::SetKeyboardFocusHere();
    _open = true;

    // Up/Down while typing move the selection (history callback).
    int move = 0;
    ImGui::SetNextItemWidth(-FLT_MIN);
    const std::string previousQuery = _query;
    const bool submitted = ImGui::InputTextWithHint("##Query", "Type a command...", &_query,
        ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackHistory, queryCallback, &move);
    if (_query != previousQuery)
        _selected = 0;

    const std::vector<const Action *> found = matches();
    if (!found.empty()) {
        _selected = std::clamp(_selected + move, 0, static_cast<int>(found.size()) - 1);
        if (move != 0)
            _scrollToSelected = true;
    }

    std::string toRun;
    if (submitted && !found.empty() && found[_selected]->enabled())
        toRun = found[_selected]->id;

    ImGui::Separator();
    const float rowHeight = ImGui::GetTextLineHeightWithSpacing();
    const float listHeight = std::min(static_cast<float>(std::max<std::size_t>(found.size(), 1)), 12.0f) * rowHeight;
    ImGui::BeginChild("##Commands", ImVec2(0.0f, listHeight + ImGui::GetStyle().WindowPadding.y));
    if (found.empty())
        ImGui::TextDisabled("No command matches \"%s\".", _query.c_str());
    for (int i = 0; i < static_cast<int>(found.size()); i++) {
        const Action &action = *found[i];
        ImGui::PushID(action.id.c_str());
        ImGui::BeginDisabled(!action.enabled());
        const std::string name = displayName(action) + (action.checked() ? "  (on)" : "");
        if (ImGui::Selectable(name.c_str(), i == _selected))
            toRun = action.id;
        if (i == _selected && _scrollToSelected) {
            ImGui::SetScrollHereY();
            _scrollToSelected = false;
        }
        const std::string shortcut = _actions.shortcutLabel(action);
        if (!shortcut.empty()) {
            ImGui::SameLine(ImGui::GetContentRegionMax().x - ImGui::CalcTextSize(shortcut.c_str()).x);
            ImGui::TextDisabled("%s", shortcut.c_str());
        }
        ImGui::EndDisabled();
        ImGui::PopID();
    }
    ImGui::EndChild();

    if (closeRequested || !toRun.empty() || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        ImGui::CloseCurrentPopup();
        _open = false;
    }
    ImGui::EndPopup();

    // After closing, so a command may open a dialog or another popup.
    if (!toRun.empty())
        run(toRun);
}
