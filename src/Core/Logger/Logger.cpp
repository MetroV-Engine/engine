#include "Core/Logger/Logger.hpp"

#include <mutex>

namespace Log {
    namespace {
        struct State {
            std::mutex mutex;
            std::vector<Entry> entries;
            std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
        };

        State& state() {
            static State instance;
            return instance;
        }
    }

    void Logger::send(Section section, Level level, std::string_view message) {
        State& s = state();
        std::lock_guard lock(s.mutex);
        s.entries.push_back(Entry{std::chrono::steady_clock::now() - s.start, level,
                                  std::string(section.name), std::string(message)});
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
