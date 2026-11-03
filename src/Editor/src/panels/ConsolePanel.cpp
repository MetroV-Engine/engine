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

#include <imgui_stdlib.h>

#include <algorithm>
#include <cctype>
#include <cfloat>
#include <cstdio>

namespace {

const ImVec4 &levelColor(LogLevel level, const Theme &theme)
{
    switch (level) {
    case LogLevel::Debug:
        return theme.colors.textMuted;
    case LogLevel::Success:
        return theme.colors.success;
    case LogLevel::Warning:
        return theme.colors.warning;
    case LogLevel::Error:
        return theme.colors.danger;
    case LogLevel::Info:
    default:
        return theme.colors.text;
    }
}

std::string toLower(std::string text)
{
    std::ranges::transform(text, text.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return text;
}

std::string formatEntry(const LogEntry &entry)
{
    char prefix[48];
    std::snprintf(prefix, sizeof(prefix), "[%8.3fs] [%s] ", entry.time, Logger::levelName(entry.level));
    std::string text = std::string(prefix) + "[" + entry.category + "] " + entry.message;
    if (entry.count > 1)
        text += " (x" + std::to_string(entry.count) + ")";
    if (!entry.file.empty())
        text += " (" + entry.file + ":" + std::to_string(entry.line) + ")";
    return text;
}

} // namespace

ConsolePanel::ConsolePanel(CommandRegistry &commands) : Panel("Console"), _commands(commands) {}

void ConsolePanel::drawContent()
{
    const Logger &logger = Logger::instance();
    if (logger.version() != _version) {
        _version = logger.version();
        logger.copyEntries(_entries);
        if (_autoScroll)
            _scrollToBottom = true;
    }

    drawToolbar();
    ImGui::Separator();

    const std::vector<const LogEntry *> visible = filteredEntries();
    // The log takes all the height except the command line.
    const float commandLineHeight = ImGui::GetFrameHeightWithSpacing() + ImGui::GetStyle().ItemSpacing.y;
    ImGui::BeginChild("##Log", ImVec2(0.0f, -commandLineHeight));
    drawLog(visible);
    ImGui::EndChild();

    ImGui::Separator();
    drawCommandLine();
}

void ConsolePanel::drawToolbar()
{
    const Theme &theme = ThemeManager::active();
    const Logger &logger = Logger::instance();

    if (Button("Clear", {}, Button::Variant::Secondary).render())
        Logger::instance().clear();
    ImGui::SameLine();
    if (Button("Copy", {}, Button::Variant::Secondary).render())
        copyToClipboard(filteredEntries());
    ImGui::SetItemTooltip("Copy the visible messages to the clipboard");
    ImGui::SameLine();
    ImGui::TextDisabled("|");

    // One toggle per level, with its count.
    static const LogLevel levels[] = {LogLevel::Debug, LogLevel::Info, LogLevel::Success, LogLevel::Warning, LogLevel::Error};
    for (const LogLevel level : levels) {
        ImGui::SameLine();
        bool &shown = _levelVisible[static_cast<int>(level)];
        const std::string label = std::string(Logger::levelName(level)) + " " + std::to_string(logger.count(level))
            + "##Level" + Logger::levelName(level);
        // Counted before the click: the button may flip `shown` below.
        int pushedColors = 1;
        ImGui::PushStyleColor(ImGuiCol_Text, shown ? levelColor(level, theme) : theme.colors.textMuted);
        if (!shown) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
            pushedColors++;
        }
        if (ImGui::Button(label.c_str()))
            shown = !shown;
        ImGui::PopStyleColor(pushedColors);
        ImGui::SetItemTooltip("%s %s messages", shown ? "Hide" : "Show", Logger::levelName(level));
    }

    // Category, filter and auto-scroll go on a second line when the panel is narrow.
    const float filtersWidth = ImGui::GetFontSize() * 30.0f;
    ImGui::SameLine();
    if (ImGui::GetContentRegionAvail().x < filtersWidth)
        ImGui::NewLine();
    const std::vector<std::string> categories = logger.categories();
    const std::size_t hidden = std::ranges::count_if(categories, [&](const std::string &c) { return _hiddenCategories.contains(c); });
    const std::string preview = hidden == 0 ? "All categories" : std::to_string(categories.size() - hidden) + " categories";
    ImGui::SetNextItemWidth(ImGui::CalcTextSize("All categories").x + ImGui::GetFrameHeight() + ImGui::GetStyle().FramePadding.x * 2.0f);
    if (ImGui::BeginCombo("##Categories", preview.c_str())) {
        for (const std::string &category : categories) {
            bool shown = !_hiddenCategories.contains(category);
            if (ImGui::Checkbox(category.c_str(), &shown)) {
                if (shown)
                    _hiddenCategories.erase(category);
                else
                    _hiddenCategories.insert(category);
            }
        }
        ImGui::EndCombo();
    }

    ImGui::SameLine();
    const float autoScrollWidth = ImGui::CalcTextSize("Auto-scroll").x + ImGui::GetFrameHeight() + ImGui::GetStyle().ItemInnerSpacing.x;
    ImGui::SetNextItemWidth(std::max(ImGui::GetFontSize() * 6.0f, ImGui::GetContentRegionAvail().x - autoScrollWidth - ImGui::GetStyle().ItemSpacing.x));
    ImGui::InputTextWithHint("##Filter", "Filter messages", &_filter);
    ImGui::SameLine();
    ImGui::Checkbox("Auto-scroll", &_autoScroll);
}

std::vector<const LogEntry *> ConsolePanel::filteredEntries() const
{
    const std::string filter = toLower(_filter);
    std::vector<const LogEntry *> visible;
    for (const LogEntry &entry : _entries) {
        if (!_levelVisible[static_cast<int>(entry.level)] || _hiddenCategories.contains(entry.category))
            continue;
        if (!filter.empty() && toLower(entry.message).find(filter) == std::string::npos
            && toLower(entry.category).find(filter) == std::string::npos)
            continue;
        visible.push_back(&entry);
    }
    return visible;
}

void ConsolePanel::drawLog(const std::vector<const LogEntry *> &visible)
{
    const Theme &theme = ThemeManager::active();
    if (visible.empty()) {
        ImGui::TextDisabled(_entries.empty() ? "No messages." : "No message matches the filters.");
        return;
    }

    const ImGuiTableFlags flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY
        | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_BordersInnerV;
    if (!ImGui::BeginTable("##LogTable", 4, flags))
        return;
    ImGui::TableSetupScrollFreeze(0, 1);
    ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize("00000.000s").x);
    ImGui::TableSetupColumn("Level", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize("Success").x);
    ImGui::TableSetupColumn("Category", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize("Performance").x);
    ImGui::TableSetupColumn("Message", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableHeadersRow();

    // Only the rows on screen are drawn, so thousands of messages stay fast.
    ImGuiListClipper clipper;
    clipper.Begin(static_cast<int>(visible.size()));
    while (clipper.Step()) {
        for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
            const LogEntry &entry = *visible[i];
            const ImVec4 &color = levelColor(entry.level, theme);
            ImGui::PushID(static_cast<int>(entry.id));
            ImGui::TableNextRow();

            ImGui::TableNextColumn();
            ImGui::TextDisabled("%.3fs", entry.time);
            ImGui::TableNextColumn();
            ImGui::TextColored(color, "%s", Logger::levelName(entry.level));
            ImGui::TableNextColumn();
            ImGui::TextDisabled("%s", entry.category.c_str());
            ImGui::TableNextColumn();
            ImGui::PushStyleColor(ImGuiCol_Text, color);
            // A selectable spanning the row gives hover feedback and a context menu.
            ImGui::Selectable(entry.message.c_str(), false, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap);
            ImGui::PopStyleColor();
            if (entry.count > 1) {
                ImGui::SameLine();
                ImGui::TextDisabled("x%d", entry.count);
            }

            if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal) && !entry.file.empty())
                ImGui::SetTooltip("%s:%d", entry.file.c_str(), entry.line);
            if (ImGui::BeginPopupContextItem("##EntryMenu")) {
                if (ImGui::MenuItem("Copy message"))
                    ImGui::SetClipboardText(entry.message.c_str());
                if (ImGui::MenuItem("Copy line"))
                    ImGui::SetClipboardText(formatEntry(entry).c_str());
                if (!entry.file.empty() && ImGui::MenuItem("Copy source location"))
                    ImGui::SetClipboardText((entry.file + ":" + std::to_string(entry.line)).c_str());
                ImGui::Separator();
                if (ImGui::MenuItem(("Hide category \"" + entry.category + "\"").c_str()))
                    _hiddenCategories.insert(entry.category);
                ImGui::EndPopup();
            }
            ImGui::PopID();
        }
    }

    // Follows new messages, unless the user scrolled up to read older ones.
    if (_scrollToBottom && ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - ImGui::GetTextLineHeight() * 2.0f)
        ImGui::SetScrollHereY(1.0f);
    _scrollToBottom = false;
    ImGui::EndTable();
}

void ConsolePanel::drawCommandLine()
{
    ImGui::AlignTextToFramePadding();
    ImGui::TextDisabled(">");
    ImGui::SameLine();
    if (_focusCommandLine) {
        ImGui::SetKeyboardFocusHere();
        _focusCommandLine = false;
    }
    ImGui::SetNextItemWidth(-FLT_MIN);
    const ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackHistory
        | ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_EscapeClearsAll;
    if (ImGui::InputTextWithHint("##Command", "Type a command (help for the list)", &_commandLine, flags,
            &ConsolePanel::commandLineCallback, this)) {
        const std::string line = _commandLine;
        _commandLine.clear();
        _historyPosition = -1;
        if (!CommandRegistry::split(line).empty()) {
            if (_history.empty() || _history.back() != line)
                _history.push_back(line);
            _scrollToBottom = true;
            _commands.execute(line);
        }
        // Keep typing commands without clicking the field again.
        ImGui::SetKeyboardFocusHere(-1);
    }
}

int ConsolePanel::commandLineCallback(ImGuiInputTextCallbackData *data)
{
    return static_cast<ConsolePanel *>(data->UserData)->handleCommandLineCallback(data);
}

int ConsolePanel::handleCommandLineCallback(ImGuiInputTextCallbackData *data)
{
    const auto replace = [data](const std::string &text) {
        data->DeleteChars(0, data->BufTextLen);
        data->InsertChars(0, text.c_str());
    };

    if (data->EventFlag == ImGuiInputTextFlags_CallbackHistory && !_history.empty()) {
        const int last = static_cast<int>(_history.size()) - 1;
        if (data->EventKey == ImGuiKey_UpArrow)
            _historyPosition = _historyPosition == -1 ? last : std::max(0, _historyPosition - 1);
        else if (data->EventKey == ImGuiKey_DownArrow && _historyPosition != -1)
            _historyPosition = _historyPosition >= last ? -1 : _historyPosition + 1;
        replace(_historyPosition == -1 ? std::string() : _history[_historyPosition]);
    } else if (data->EventFlag == ImGuiInputTextFlags_CallbackCompletion) {
        // Completes the command name (first word) only.
        const std::string text(data->Buf, data->BufTextLen);
        if (text.find(' ') != std::string::npos)
            return 0;
        const std::vector<std::string> matches = _commands.complete(text);
        if (matches.size() == 1) {
            replace(matches[0] + " ");
        } else if (matches.size() > 1) {
            // Complete the common prefix and list the candidates.
            std::string common = matches[0];
            for (const std::string &match : matches)
                while (!match.starts_with(common))
                    common.pop_back();
            replace(common);
            std::string list;
            for (const std::string &match : matches)
                list += (list.empty() ? "" : "  ") + match;
            Logger::instance().info("Console", list);
        }
    }
    return 0;
}

void ConsolePanel::copyToClipboard(const std::vector<const LogEntry *> &entries) const
{
    std::string text;
    for (const LogEntry *entry : entries)
        text += formatEntry(*entry) + "\n";
    ImGui::SetClipboardText(text.c_str());
}
