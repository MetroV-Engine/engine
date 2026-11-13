#pragma once

#include <chrono>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace Log {
    /** @brief Severity of a message, lowest first. */
    enum class Level : int { Debug, Info, Success, Warning, Error };

    /** @brief Name of the module that sends a message. Each module declares its own, once. */
    struct Section {
        std::string_view name;
    };

    /** @brief One stored message. */
    struct Entry {
        /** @brief Time since the logger was first used. */
        std::chrono::steady_clock::duration timestamp;
        Level level;
        std::string section;
        std::string message;
    };

    /** @brief Static facade over the engine-wide logger. */
    class Logger {
    public:
        /** @brief Stores the message and prints it as `[Level][Section] message`. Never throws. */
        static void send(Section section, Level level, std::string_view message);
        /** @brief True when a message of this level would be kept. */
        static bool isEnabled(Level level);
        /** @brief Messages below this level are discarded. Default: Debug. */
        static void setMinLevel(Level level);
        /** @brief Empties the buffer and gives it room for this many entries (0 counts as 1). Default: 1024. */
        static void setCapacity(std::size_t entries);
        /** @brief Copy of the stored messages, oldest first. */
        static std::vector<Entry> snapshot();
        /** @brief Messages overwritten because the buffer was full, since the last clear. */
        static std::size_t droppedCount();
        /** @brief Empties the buffer and resets the dropped count. */
        static void clear();
    };
}
