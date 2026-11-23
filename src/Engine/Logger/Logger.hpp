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
#ifdef ENGINE_LOG_ENABLED
        /** @brief Stores the message and prints it as `[Level][Section] message`. Never throws. */
        static void send(Section section, Level level, std::string_view message);
        /** @brief True when a message of this level would be kept. */
        static bool isEnabled(Level level);
#else
        // Logging is compiled out: the call does nothing (its arguments still run; ENGINE_LOG skips them).
        static void send(Section, Level, std::string_view) {}
        static bool isEnabled(Level) { return false; }
#endif
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

#ifdef ENGINE_LOG_ENABLED
/**
 * @brief Logs a message: `ENGINE_LOG(ECS, Log::Level::Info, "entity spawned");`
 *
 * The level is evaluated once. The message is evaluated only when its level
 * passes the minimum level, so a filtered message is never built.
 */
#define ENGINE_LOG(section, level, message)                              \
    do {                                                                 \
        const ::Log::Level engineLogLevel_ = (level);                    \
        if (::Log::Logger::isEnabled(engineLogLevel_)) {                 \
            ::Log::Logger::send((section), engineLogLevel_, (message));  \
        }                                                                \
    } while (false)
#else
// Logging is compiled out. sizeof does not evaluate its operand, so nothing
// runs; but the call to send inside it is still type-checked, so wrong
// arguments fail here exactly as in Debug, and variables used only in a log
// call are not reported as unused.
#define ENGINE_LOG(section, level, message)                                         \
    do {                                                                            \
        static_cast<void>(sizeof((::Log::Logger::send((section), (level), (message)), 0))); \
    } while (false)
#endif
