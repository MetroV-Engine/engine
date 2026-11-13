#include <boost/ut.hpp>

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

    "setMinLevel changes while others send"_test = [] {
        ScopedLoggerTest scope;
        Log::Logger::setCapacity(10000);

        std::vector<std::thread> threads;
        for (int t = 0; t < 2; ++t) {
            threads.emplace_back([] {
                for (int i = 0; i < 2000; ++i) {
                    Log::Logger::send(Log::Section{"ECS"}, Log::Level::Info, "w" + std::to_string(i));
                }
            });
        }
        threads.emplace_back([] {
            for (int i = 0; i < 1000; ++i) {
                Log::Logger::setMinLevel(i % 2 == 0 ? Log::Level::Error : Log::Level::Info);
            }
        });
        for (auto& thread : threads) {
            thread.join();
        }

        std::size_t wrong = 0;
        for (const auto& entry : Log::Logger::snapshot()) {
            if (entry.level != Log::Level::Info || entry.message.empty() || entry.message[0] != 'w') {
                ++wrong;
            }
        }
        expect(wrong == 0_u);
    };

    "clear and snapshot while others send"_test = [] {
        ScopedLoggerTest scope;
        Log::Logger::setCapacity(10000);

        std::vector<std::thread> senders;
        for (int t = 0; t < 2; ++t) {
            senders.emplace_back([] {
                for (int i = 0; i < 2000; ++i) {
                    Log::Logger::send(Log::Section{"ECS"}, Log::Level::Info, "w" + std::to_string(i));
                }
            });
        }
        std::size_t incomplete = 0;
        for (int round = 0; round < 50; ++round) {
            for (const auto& entry : Log::Logger::snapshot()) {
                if (entry.section != "ECS" || entry.message.empty() || entry.message[0] != 'w') {
                    ++incomplete;
                }
            }
            Log::Logger::clear();
        }
        for (auto& thread : senders) {
            thread.join();
        }

        expect(incomplete == 0_u);
    };
}
