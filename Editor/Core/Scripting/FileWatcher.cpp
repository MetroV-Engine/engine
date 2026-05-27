/*
    Created on: 3/10/26
    Filename: FileWatcher.cpp
    Description: Polls filesystem modification times for tracked paths.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "FileWatcher.hpp"

namespace Editor::Scripting {

    // Maps (wasExisting, nowExisting) → FileStatus.
    // No entry means nothing happened (both false = still gone).
    static const std::unordered_map<int, FileStatus> kTransitionTable = {
        { 0b01, FileStatus::Created  },   // false → true
        { 0b10, FileStatus::Removed  },   // true  → false
        { 0b11, FileStatus::Modified },   // true  → true  (checked separately)
    };

    void FileWatcher::watch(const std::string &path, Callback callback)
    {
        Entry entry;
        entry.callback      = std::move(callback);
        entry.exists        = std::filesystem::exists(path);
        entry.lastWriteTime = entry.exists
                              ? std::filesystem::last_write_time(path)
                              : std::filesystem::file_time_type{};
        _entries[path] = std::move(entry);
    }

    void FileWatcher::unwatch(const std::string &path)
    {
        _entries.erase(path);
    }

    bool FileWatcher::isWatching(const std::string &path) const
    {
        return _entries.count(path) > 0;
    }

    void FileWatcher::tick()
    {
        for (auto &[path, entry] : _entries) {
            const bool nowExists = std::filesystem::exists(path);
            const int  key       = (entry.exists ? 0b10 : 0) | (nowExists ? 0b01 : 0);

            auto it = kTransitionTable.find(key);
            if (it == kTransitionTable.end()) continue;  // 0b00 — still missing, skip

            if (it->second == FileStatus::Modified) {
                const auto newTime = std::filesystem::last_write_time(path);
                if (newTime == entry.lastWriteTime) continue;
                entry.lastWriteTime = newTime;
            }

            entry.exists = nowExists;
            if (nowExists) entry.lastWriteTime = std::filesystem::last_write_time(path);

            entry.callback(path, it->second);
        }
    }

} // Editor::Scripting





