/*
    Created on: 30/03/2026
    Filename: Logger.hpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>
#include <fstream>
#include <mutex>
#include <source_location>
#include <string>
#include <vector>

enum class LogLevel
{
    Debug,
    Info,
    Success,
    Warning,
    Error,
};

struct LogEntry
{
    std::uint64_t id;
    double time; // Seconds since the logger started
    LogLevel level;
    std::string category; // Who logged it: "Editor", "SDL", "Engine", "Physics"...
    std::string message;
    std::string file; // Source location of the log call (empty for external logs)
    int line = 0;
    // Identical consecutive messages are merged: number of repetitions.
    int count = 1;
};

// Central log of the editor and the engine. Any code (any thread) logs here;
// the developer console shows it, and it is also written to the terminal and
// to a log file.
//
//   Logger::instance().warning("Physics", "Body has zero mass");
//
// SDL's own logs (SDL_Log...) are redirected here with captureSdlLogs().
class Logger
{
public:
    static constexpr std::size_t MaxEntries = 5000;

    static Logger &instance();

    void log(LogLevel level, std::string category, std::string message,
        std::source_location location = std::source_location::current());
    void debug(std::string category, std::string message, std::source_location location = std::source_location::current());
    void info(std::string category, std::string message, std::source_location location = std::source_location::current());
    void success(std::string category, std::string message, std::source_location location = std::source_location::current());
    void warning(std::string category, std::string message, std::source_location location = std::source_location::current());
    void error(std::string category, std::string message, std::source_location location = std::source_location::current());

    // Copies the entries (oldest first). Cheap to call every frame: compare
    // version() first and copy only when it changed.
    void copyEntries(std::vector<LogEntry> &out) const;
    // Changes whenever an entry is added, merged or the log is cleared.
    std::uint64_t version() const;
    // Number of messages of a level since the last clear (repetitions included).
    std::size_t count(LogLevel level) const;
    // Every category seen so far, sorted.
    std::vector<std::string> categories() const;
    void clear();

    // Messages below this level are not printed in the terminal (all are kept
    // in the console). Debug by default.
    void setTerminalLevel(LogLevel level);
    // Writes every message to this file (truncated). Empty path disables it.
    bool setLogFile(const std::string &path);
    // Routes SDL_Log/SDL_LogError... into this logger, category "SDL".
    void captureSdlLogs();

    static const char *levelName(LogLevel level);

private:
    Logger();

    mutable std::mutex _mutex;
    std::deque<LogEntry> _entries;
    std::uint64_t _nextId = 1;
    std::uint64_t _version = 0;
    std::size_t _counts[5] = {};
    std::vector<std::string> _categories;
    LogLevel _terminalLevel = LogLevel::Debug;
    std::ofstream _file;
    std::uint64_t _startTicks;

    void write(const LogEntry &entry);
};
