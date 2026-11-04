/*
    Created on: 30/03/2026
    Filename: ConsolePanel.hpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#pragma once

#include "panels/Panel/Panel.hpp"
#include "core/CommandRegistry/CommandRegistry.hpp"
#include "core/Logger/Logger.hpp"

#include <cstdint>
#include <set>
#include <string>
#include <utility>
#include <vector>

// Developer console: every log of the editor and the engine (from the
// Logger) in a table, with filters by level, category and text, plus a
// command line ("help" lists the commands; Up/Down for history, Tab completes).
class ConsolePanel : public Panel
{
public:
    explicit ConsolePanel(CommandRegistry &commands);

    // Shows or hides a level in the table (Debug is hidden by default).
    void setLevelVisible(LogLevel level, bool visible) { _levelVisible[static_cast<int>(level)] = visible; }
    bool isLevelVisible(LogLevel level) const { return _levelVisible[static_cast<int>(level)]; }
    const std::set<std::string> &getHiddenCategories() const { return _hiddenCategories; }
    void setHiddenCategories(std::set<std::string> categories) { _hiddenCategories = std::move(categories); }
    bool isAutoScroll() const { return _autoScroll; }
    void setAutoScroll(bool autoScroll) { _autoScroll = autoScroll; }
    // Puts the keyboard focus on the command line on the next frame.
    void focusCommandLine() { _focusCommandLine = true; }

protected:
    void drawContent() override;

private:
    CommandRegistry &_commands;

    // Last copy of the Logger's entries, refreshed when its version changes.
    std::vector<LogEntry> _entries;
    std::uint64_t _version = ~std::uint64_t(0);

    bool _levelVisible[5] = {false, true, true, true, true};
    // Categories the user unchecked (new categories are shown by default).
    std::set<std::string> _hiddenCategories;
    std::string _filter;
    bool _autoScroll = true;
    bool _scrollToBottom = false;

    std::string _commandLine;
    std::vector<std::string> _history;
    int _historyPosition = -1; // -1: not browsing the history
    bool _focusCommandLine = false;

    void drawToolbar();
    void drawLog(const std::vector<const LogEntry *> &visible);
    void drawCommandLine();
    std::vector<const LogEntry *> filteredEntries() const;
    void copyToClipboard(const std::vector<const LogEntry *> &entries) const;
    static int commandLineCallback(ImGuiInputTextCallbackData *data);
    int handleCommandLineCallback(ImGuiInputTextCallbackData *data);
};
