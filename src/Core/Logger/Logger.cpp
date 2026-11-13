#include "Core/Logger/Logger.hpp"

#include <atomic>
#include <mutex>

namespace Log {
    namespace {
        constexpr std::size_t kDefaultCapacity = 1024;

        struct State {
            std::mutex mutex;
            std::vector<Entry> ring = std::vector<Entry>(kDefaultCapacity);
            std::size_t head = 0;     // next slot to write
            std::size_t count = 0;    // stored entries, at most ring.size()
            std::size_t dropped = 0;  // entries overwritten since the last clear
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
        if (s.count == s.ring.size()) {
            ++s.dropped;
        } else {
            ++s.count;
        }
        s.ring[s.head] = Entry{std::chrono::steady_clock::now() - s.start, level,
                               std::string(section.name), std::string(message)};
        s.head = (s.head + 1) % s.ring.size();
    }

    bool Logger::isEnabled(Level level) {
        return level >= state().minLevel.load(std::memory_order_relaxed);
    }

    void Logger::setMinLevel(Level level) {
        state().minLevel.store(level, std::memory_order_relaxed);
    }

    void Logger::setCapacity(std::size_t entries) {
        // Allocated before the lock: an impossible size throws with the buffer untouched.
        std::vector<Entry> fresh(entries == 0 ? 1 : entries);
        State& s = state();
        std::lock_guard lock(s.mutex);
        s.ring.swap(fresh);
        s.head = 0;
        s.count = 0;
        s.dropped = 0;
    }

    std::size_t Logger::droppedCount() {
        State& s = state();
        std::lock_guard lock(s.mutex);
        return s.dropped;
    }

    std::vector<Entry> Logger::snapshot() {
        State& s = state();
        std::lock_guard lock(s.mutex);
        std::vector<Entry> entries;
        entries.reserve(s.count);
        const std::size_t oldest = (s.head + s.ring.size() - s.count) % s.ring.size();
        for (std::size_t i = 0; i < s.count; ++i) {
            entries.push_back(s.ring[(oldest + i) % s.ring.size()]);
        }
        return entries;
    }

    void Logger::clear() {
        State& s = state();
        std::lock_guard lock(s.mutex);
        s.head = 0;
        s.count = 0;
        s.dropped = 0;
    }
}
