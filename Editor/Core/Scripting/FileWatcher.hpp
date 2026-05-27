/*
    Created on: 3/10/26
    Filename: FileWatcher.hpp
    Description: Polls filesystem modification times for a set of tracked paths.
                 Fires an onChange callback when a file is created, modified, or removed.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#ifndef PROTO_FILEWATCHER_HPP
#define PROTO_FILEWATCHER_HPP

#include <filesystem>
#include <functional>
#include <string>
#include <unordered_map>

namespace Editor::Scripting {

    enum class FileStatus { Created, Modified, Removed };

    class FileWatcher {
    public:
        using Callback = std::function<void(const std::string &path, FileStatus)>;

        // Register a file path to watch and the callback to fire on change
        void watch(const std::string &path, Callback callback);

        // Stop watching a path
        void unwatch(const std::string &path);

        // Returns true if this path is already registered
        [[nodiscard]] bool isWatching(const std::string &path) const;

        // Call once per frame — checks all tracked paths for changes
        void tick();

    private:
        struct Entry {
            std::filesystem::file_time_type lastWriteTime;
            bool                            exists = false;
            Callback                        callback;
        };

        std::unordered_map<std::string, Entry> _entries;
    };

} // Editor::Scripting

// Hash specialization so FileStatus can be used as an unordered_map key
namespace std {
    template <>
    struct hash<Editor::Scripting::FileStatus> {
        std::size_t operator()(Editor::Scripting::FileStatus s) const noexcept {
            return std::hash<int>{}(static_cast<int>(s));
        }
    };
}

#endif //PROTO_FILEWATCHER_HPP


