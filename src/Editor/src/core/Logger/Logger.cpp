/*
    Created on: 30/03/2026
    Filename: Logger.cpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "Logger.hpp"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <utility>

namespace {

// Path relative to the project, to keep log lines short.
std::string shortPath(const char *file)
{
    const std::string path = file;
    for (const char *root : {"/src/", "/main.cpp"}) {
        const std::size_t position = path.rfind(root);
        if (position != std::string::npos)
            return path.substr(position + 1);
    }
    return path;
}

LogLevel fromSdlPriority(SDL_LogPriority priority)
{
    switch (priority) {
    case SDL_LOG_PRIORITY_TRACE:
    case SDL_LOG_PRIORITY_VERBOSE:
    case SDL_LOG_PRIORITY_DEBUG:
        return LogLevel::Debug;
    case SDL_LOG_PRIORITY_WARN:
        return LogLevel::Warning;
    case SDL_LOG_PRIORITY_ERROR:
    case SDL_LOG_PRIORITY_CRITICAL:
        return LogLevel::Error;
    case SDL_LOG_PRIORITY_INFO:
    default:
        return LogLevel::Info;
    }
}

} // namespace

Logger &Logger::instance()
{
    static Logger logger;
    return logger;
}

Logger::Logger() : _startTicks(SDL_GetTicksNS()) {}

const char *Logger::levelName(LogLevel level)
{
    switch (level) {
    case LogLevel::Debug:
        return "Debug";
    case LogLevel::Info:
        return "Info";
    case LogLevel::Success:
        return "Success";
    case LogLevel::Warning:
        return "Warning";
    case LogLevel::Error:
    default:
        return "Error";
    }
}

void Logger::log(LogLevel level, std::string category, std::string message, std::source_location location)
{
    const double time = static_cast<double>(SDL_GetTicksNS() - _startTicks) / 1e9;
    std::lock_guard lock(_mutex);
    _counts[static_cast<int>(level)]++;
    _version++;

    // A message repeated in a row (e.g. every frame) is shown once with a counter.
    if (!_entries.empty()) {
        LogEntry &last = _entries.back();
        if (last.level == level && last.category == category && last.message == message) {
            last.count++;
            last.time = time;
            return;
        }
    }

    if (!std::ranges::binary_search(_categories, category))
        _categories.insert(std::ranges::upper_bound(_categories, category), category);

    LogEntry entry{_nextId++, time, level, std::move(category), std::move(message),
        location.line() > 0 ? shortPath(location.file_name()) : std::string(), static_cast<int>(location.line())};
    write(entry);
    _entries.push_back(std::move(entry));
    if (_entries.size() > MaxEntries)
        _entries.pop_front();
}

void Logger::write(const LogEntry &entry)
{
    char prefix[64];
    std::snprintf(prefix, sizeof(prefix), "[%8.3fs] [%-7s] ", entry.time, levelName(entry.level));
    std::string line = std::string(prefix) + "[" + entry.category + "] " + entry.message;
    if (entry.level >= LogLevel::Warning && !entry.file.empty())
        line += " (" + entry.file + ":" + std::to_string(entry.line) + ")";

    if (entry.level >= _terminalLevel)
        (entry.level >= LogLevel::Warning ? std::cerr : std::cout) << line << '\n' << std::flush;
    if (_file.is_open())
        _file << line << '\n' << std::flush;
}

void Logger::debug(std::string category, std::string message, std::source_location location)
{
    log(LogLevel::Debug, std::move(category), std::move(message), location);
}

void Logger::info(std::string category, std::string message, std::source_location location)
{
    log(LogLevel::Info, std::move(category), std::move(message), location);
}

void Logger::success(std::string category, std::string message, std::source_location location)
{
    log(LogLevel::Success, std::move(category), std::move(message), location);
}

void Logger::warning(std::string category, std::string message, std::source_location location)
{
    log(LogLevel::Warning, std::move(category), std::move(message), location);
}

void Logger::error(std::string category, std::string message, std::source_location location)
{
    log(LogLevel::Error, std::move(category), std::move(message), location);
}

void Logger::copyEntries(std::vector<LogEntry> &out) const
{
    std::lock_guard lock(_mutex);
    out.assign(_entries.begin(), _entries.end());
}

std::uint64_t Logger::version() const
{
    std::lock_guard lock(_mutex);
    return _version;
}

std::size_t Logger::count(LogLevel level) const
{
    std::lock_guard lock(_mutex);
    return _counts[static_cast<int>(level)];
}

std::vector<std::string> Logger::categories() const
{
    std::lock_guard lock(_mutex);
    return _categories;
}

void Logger::clear()
{
    std::lock_guard lock(_mutex);
    _entries.clear();
    std::ranges::fill(_counts, 0);
    _version++;
}

void Logger::setTerminalLevel(LogLevel level)
{
    std::lock_guard lock(_mutex);
    _terminalLevel = level;
}

bool Logger::setLogFile(const std::string &path)
{
    std::lock_guard lock(_mutex);
    _file.close();
    if (path.empty())
        return true;
    std::error_code error;
    const std::filesystem::path parent = std::filesystem::path(path).parent_path();
    if (!parent.empty())
        std::filesystem::create_directories(parent, error);
    _file.open(path, std::ios::trunc);
    return _file.is_open();
}

void Logger::captureSdlLogs()
{
    SDL_SetLogOutputFunction(
        [](void *, int, SDL_LogPriority priority, const char *message) {
            // No source location: the call happened inside SDL or through SDL_Log.
            Logger::instance().log(fromSdlPriority(priority), "SDL", message, std::source_location());
        },
        nullptr);
}
