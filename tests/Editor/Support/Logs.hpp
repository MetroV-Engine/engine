#pragma once

#include "core/Logger/Logger.hpp"

#include <string_view>
#include <vector>

// True if the Logger holds an entry of this level whose message contains text.
// The Logger is process-wide: tests call Logger::instance().clear() first.
inline bool logged(LogLevel level, std::string_view text)
{
    std::vector<LogEntry> entries;
    Logger::instance().copyEntries(entries);
    for (const LogEntry &entry : entries)
        if (entry.level == level && entry.message.find(text) != std::string::npos)
            return true;
    return false;
}

inline std::size_t loggedErrors()
{
    return Logger::instance().count(LogLevel::Error);
}
