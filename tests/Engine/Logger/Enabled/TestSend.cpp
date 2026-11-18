#include <boost/ut.hpp>

#include <chrono>
#include <string>
#include <thread>

#include "LoggerTestSupport.hpp"

using namespace boost::ut;

void run_logger_send_tests() {
    "send stores level, section and message"_test = [] {
        ScopedLoggerTest scope;
        Log::Logger::send(Log::Section{"ECS"}, Log::Level::Warning, "entity spawned");
        const auto entries = Log::Logger::snapshot();
        expect((entries.size() == 1_u) >> fatal);
        expect(entries[0].level == Log::Level::Warning);
        expect(entries[0].section == "ECS");
        expect(entries[0].message == "entity spawned");
    };

    "a message sent 5 ms later has a timestamp at least 5 ms later"_test = [] {
        ScopedLoggerTest scope;
        Log::Logger::send(Log::Section{"ECS"}, Log::Level::Info, "first");
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        Log::Logger::send(Log::Section{"ECS"}, Log::Level::Info, "second");
        const auto entries = Log::Logger::snapshot();
        expect((entries.size() == 2_u) >> fatal);
        expect(entries[1].timestamp - entries[0].timestamp >= std::chrono::milliseconds(5));
    };

    "empty message is stored"_test = [] {
        ScopedLoggerTest scope;
        Log::Logger::send(Log::Section{"ECS"}, Log::Level::Info, "");
        const auto entries = Log::Logger::snapshot();
        expect((entries.size() == 1_u) >> fatal);
        expect(entries[0].message.empty());
    };

    "empty section name is stored"_test = [] {
        ScopedLoggerTest scope;
        Log::Logger::send(Log::Section{""}, Log::Level::Info, "hello");
        const auto entries = Log::Logger::snapshot();
        expect((entries.size() == 1_u) >> fatal);
        expect(entries[0].section.empty());
        expect(entries[0].message == "hello");
    };

    "UTF-8 message is stored byte for byte"_test = [] {
        ScopedLoggerTest scope;
        Log::Logger::send(Log::Section{"ECS"}, Log::Level::Info, "élément créé à 30 °C");
        const auto entries = Log::Logger::snapshot();
        expect((entries.size() == 1_u) >> fatal);
        expect(entries[0].message == "élément créé à 30 °C");
    };

    "100 000 character message is stored intact"_test = [] {
        ScopedLoggerTest scope;
        const std::string big(100000, 'x');
        Log::Logger::send(Log::Section{"ECS"}, Log::Level::Info, big);
        const auto entries = Log::Logger::snapshot();
        expect((entries.size() == 1_u) >> fatal);
        expect(entries[0].message.size() == 100000_u);
        expect(entries[0].message == big);
    };

    "newline and tab are stored unchanged"_test = [] {
        ScopedLoggerTest scope;
        Log::Logger::send(Log::Section{"ECS"}, Log::Level::Info, "line one\n\tline two");
        const auto entries = Log::Logger::snapshot();
        expect((entries.size() == 1_u) >> fatal);
        expect(entries[0].message == "line one\n\tline two");
    };

    "a snapshot is a copy that survives clear"_test = [] {
        ScopedLoggerTest scope;
        Log::Logger::send(Log::Section{"ECS"}, Log::Level::Info, "kept");
        const auto entries = Log::Logger::snapshot();
        Log::Logger::clear();
        expect((entries.size() == 1_u) >> fatal);
        expect(entries[0].message == "kept");
    };

    "clear empties the buffer"_test = [] {
        ScopedLoggerTest scope;
        Log::Logger::send(Log::Section{"ECS"}, Log::Level::Info, "a");
        Log::Logger::send(Log::Section{"ECS"}, Log::Level::Info, "b");
        expect((Log::Logger::snapshot().size() == 2_u) >> fatal);
        Log::Logger::clear();
        expect(Log::Logger::snapshot().empty());
    };
}
