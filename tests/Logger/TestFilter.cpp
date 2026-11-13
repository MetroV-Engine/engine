#include <boost/ut.hpp>

#include "LoggerTestSupport.hpp"

using namespace boost::ut;

namespace {
    void sendOneOfEachLevel() {
        Log::Logger::send(Log::Section{"ECS"}, Log::Level::Debug, "debug");
        Log::Logger::send(Log::Section{"ECS"}, Log::Level::Info, "info");
        Log::Logger::send(Log::Section{"ECS"}, Log::Level::Success, "success");
        Log::Logger::send(Log::Section{"ECS"}, Log::Level::Warning, "warning");
        Log::Logger::send(Log::Section{"ECS"}, Log::Level::Error, "error");
    }
}

void run_logger_filter_tests() {
    "by default every level is enabled and stored"_test = [] {
        ScopedLoggerTest scope;
        expect(Log::Logger::isEnabled(Log::Level::Debug));
        expect(Log::Logger::isEnabled(Log::Level::Error));
        sendOneOfEachLevel();
        expect(Log::Logger::snapshot().size() == 5_u);
    };

    "minimum Warning keeps Warning and Error only"_test = [] {
        ScopedLoggerTest scope;
        Log::Logger::setMinLevel(Log::Level::Warning);
        sendOneOfEachLevel();
        const auto entries = Log::Logger::snapshot();
        expect((entries.size() == 2_u) >> fatal);
        expect(entries[0].message == "warning");
        expect(entries[1].message == "error");
    };

    "isEnabled agrees with the minimum level"_test = [] {
        ScopedLoggerTest scope;
        Log::Logger::setMinLevel(Log::Level::Success);
        expect(!Log::Logger::isEnabled(Log::Level::Info));
        expect(Log::Logger::isEnabled(Log::Level::Success));
        expect(Log::Logger::isEnabled(Log::Level::Error));
    };
}
