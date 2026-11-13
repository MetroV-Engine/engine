#include "Core/Logger/Logger.hpp"

#include <atomic>
#include <mutex>

namespace Log {
    namespace {
        struct State {
            std::mutex mutex;
            std::vector<Entry> entries;
            std::atomic<Level> minLevel{Level::Debug};
            std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
        };

        State& state() {
            static State instance;
            return instance;
        }
    }

    void Logger::send(Section section, Level level, std::string_view message) {
        if (!isEnabled(level)) {
            return;
        }
        State& s = state();
        std::lock_guard lock(s.mutex);
        s.entries.push_back(Entry{std::chrono::steady_clock::now() - s.start, level,
                                  std::string(section.name), std::string(message)});
    }

    bool Logger::isEnabled(Level level) {
        return level >= state().minLevel.load(std::memory_order_relaxed);
    }

    void Logger::setMinLevel(Level level) {
        state().minLevel.store(level, std::memory_order_relaxed);
    }

    std::vector<Entry> Logger::snapshot() {
        State& s = state();
        std::lock_guard lock(s.mutex);
        return s.entries;
    }

    void Logger::clear() {
        State& s = state();
        std::lock_guard lock(s.mutex);
        s.entries.clear();
    }
}
