#include "Core/Logger/Logger.hpp"

#include <atomic>
#include <iostream>
#include <mutex>

namespace Log {
    namespace {
        constexpr std::size_t kDefaultCapacity = 1024;

        // Indexed by Level, in declaration order.
        constexpr std::string_view kLevelNames[] = {"Debug", "Info", "Success", "Warning", "Error"};

        // "[Level][Section] message\n", built whole so it can be written in one call.
        std::string formatLine(Section section, Level level, std::string_view message) {
            const std::string_view name = kLevelNames[static_cast<int>(level)];
            std::string line;
            line.reserve(name.size() + section.name.size() + message.size() + 5);
            line += '[';
            line += name;
            line += "][";
            line += section.name;
            line += "] ";
            line += message;
            line += '\n';
            return line;
        }

        struct State {
            std::mutex mutex;        // guards the buffer
            std::mutex outputMutex;  // keeps each printed line whole
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
        // Nothing may escape: a message or line that cannot be built or written is dropped.
        try {
            State& s = state();
            // Built before any lock: if a copy fails, the buffer has not been touched.
            Entry entry{std::chrono::steady_clock::now() - s.start, level,
                        std::string(section.name), std::string(message)};
            const std::string line = formatLine(section, level, message);
            {
                std::lock_guard lock(s.mutex);
                s.ring[s.head] = std::move(entry);
                s.head = (s.head + 1) % s.ring.size();
                if (s.count == s.ring.size()) {
                    ++s.dropped;
                } else {
                    ++s.count;
                }
            }

            std::ostream& out = level >= Level::Warning ? std::cerr : std::cout;
            std::lock_guard outputLock(s.outputMutex);
            out << line;
        } catch (...) {
        }
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
