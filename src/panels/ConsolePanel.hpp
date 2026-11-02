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

#include "Panel.hpp"
#include "components/Checkbox.hpp"
#include "components/TextInput.hpp"

#include <cstddef>
#include <deque>
#include <string>

// Scrollable log of editor messages, with a level color, a timestamp, a text
// filter and auto-scroll. Keeps the last MaxEntries messages.
class ConsolePanel : public Panel
{
public:
    enum class Level
    {
        Info,
        Success,
        Warning,
        Error,
    };

    static constexpr std::size_t MaxEntries = 1000;

    ConsolePanel();

    void log(Level level, std::string message);
    void clear() { _entries.clear(); }
    std::size_t size() const { return _entries.size(); }

protected:
    void drawContent() override;

private:
    struct Entry
    {
        double time; // Seconds since the editor started
        Level level;
        std::string message;
    };

    std::deque<Entry> _entries;
    TextInput _filter;
    Checkbox _autoScroll;
};
