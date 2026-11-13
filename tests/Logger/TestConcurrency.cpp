#include <boost/ut.hpp>

#include <atomic>
#include <chrono>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "LoggerTestSupport.hpp"

using namespace boost::ut;

namespace {
    std::vector<std::string> splitLines(const std::string& text) {
        std::vector<std::string> lines;
        std::istringstream stream(text);
        for (std::string line; std::getline(stream, line);) {
            lines.push_back(line);
        }
        return lines;
    }
}

void run_logger_concurrency_tests() {
    "4 threads x 1000 messages: all stored, every printed line whole"_test = [] {
        ScopedLoggerTest scope;
        Log::Logger::setCapacity(10000);

        std::vector<std::thread> threads;
        for (int t = 0; t < 4; ++t) {
            threads.emplace_back([t] {
                for (int i = 0; i < 1000; ++i) {
                    Log::Logger::send(Log::Section{"ECS"}, Log::Level::Info,
                                      "t" + std::to_string(t) + "-" + std::to_string(i));
                }
            });
        }
        for (auto& thread : threads) {
            thread.join();
        }

        expect(Log::Logger::snapshot().size() == 4000_u);
        expect(Log::Logger::droppedCount() == 0_u);

        const auto lines = splitLines(scope.stdoutText());
        const std::regex wholeLine(R"(\[Info\]\[ECS\] t[0-3]-[0-9]+)");
        std::size_t malformed = 0;
        for (const auto& line : lines) {
            if (!std::regex_match(line, wholeLine)) {
                ++malformed;
            }
        }
        const std::set<std::string> distinct(lines.begin(), lines.end());
        expect(lines.size() == 4000_u);
        expect(malformed == 0_u);
        expect(distinct.size() == 4000_u);
    };

    "setMinLevel changes while others send: some messages kept, some filtered"_test = [] {
        ScopedLoggerTest scope;
        Log::Logger::setCapacity(50000);
        constexpr int perSender = 20000;
        std::atomic<bool> go{false};
        std::atomic<int> sendersDone{0};

        std::vector<std::thread> threads;
        for (int t = 0; t < 2; ++t) {
            threads.emplace_back([&] {
                while (!go) {
                    std::this_thread::yield();
                }
                for (int i = 0; i < perSender; ++i) {
                    Log::Logger::send(Log::Section{"ECS"}, Log::Level::Info, "w" + std::to_string(i));
                }
                ++sendersDone;
            });
        }
        threads.emplace_back([&] {
            while (!go) {
                std::this_thread::yield();
            }
            for (int i = 0; sendersDone < 2; ++i) {
                Log::Logger::setMinLevel(i % 2 == 0 ? Log::Level::Error : Log::Level::Info);
            }
        });
        go = true;
        for (auto& thread : threads) {
            thread.join();
        }

        const auto entries = Log::Logger::snapshot();
        std::size_t wrong = 0;
        for (const auto& entry : entries) {
            if (entry.level != Log::Level::Info || entry.message.empty() || entry.message[0] != 'w') {
                ++wrong;
            }
        }
        // Overlap really happened: the filter both let messages through and stopped some.
        expect(entries.size() > 0_u);
        expect(entries.size() < 40000_u);
        expect(wrong == 0_u);
    };

    "clear and snapshot while others send"_test = [] {
        ScopedLoggerTest scope;
        // A small buffer makes the senders keep overwriting the slots being read.
        Log::Logger::setCapacity(64);
        const std::string padding(40, 'x');  // heap-allocated messages, so a torn copy shows
        std::atomic<bool> go{false};
        std::atomic<bool> stop{false};

        std::vector<std::thread> senders;
        for (int t = 0; t < 2; ++t) {
            senders.emplace_back([&] {
                while (!go) {
                    std::this_thread::yield();
                }
                for (int i = 0; !stop; ++i) {
                    Log::Logger::send(Log::Section{"ECS"}, Log::Level::Info, "w" + padding + std::to_string(i));
                }
            });
        }
        go = true;

        // Read while the senders run until 2000 reads saw messages (3 s at most).
        std::size_t nonEmptySnapshots = 0;
        std::size_t incomplete = 0;
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
        for (int round = 0; nonEmptySnapshots < 2000 && std::chrono::steady_clock::now() < deadline; ++round) {
            const auto entries = Log::Logger::snapshot();
            if (!entries.empty()) {
                ++nonEmptySnapshots;
            }
            for (const auto& entry : entries) {
                const bool whole = entry.section == "ECS" && entry.message.size() > padding.size() + 1 &&
                                   entry.message[0] == 'w' && entry.message.compare(1, padding.size(), padding) == 0;
                if (!whole) {
                    ++incomplete;
                }
            }
            if (round % 10 == 9) {
                Log::Logger::clear();
            }
        }
        stop = true;
        for (auto& thread : senders) {
            thread.join();
        }

        expect(nonEmptySnapshots >= 2000_u);
        expect(incomplete == 0_u);
    };
}
