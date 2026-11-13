#include <boost/ut.hpp>

#include <limits>
#include <string>

#include "LoggerTestSupport.hpp"

using namespace boost::ut;

namespace {
    void sendTimes(int count, const char* text) {
        for (int i = 0; i < count; ++i) {
            Log::Logger::send(Log::Section{"ECS"}, Log::Level::Info, text);
        }
    }
}

void run_logger_capacity_tests() {
    "setCapacity clears the buffer and applies the new size"_test = [] {
        ScopedLoggerTest scope;
        sendTimes(5, "old");
        expect((Log::Logger::snapshot().size() == 5_u) >> fatal);
        Log::Logger::setCapacity(3);
        expect(Log::Logger::snapshot().empty());
        sendTimes(5, "new");
        expect(Log::Logger::snapshot().size() == 3_u);
        expect(Log::Logger::droppedCount() == 2_u);
    };

    "setCapacity(0) behaves as capacity 1"_test = [] {
        ScopedLoggerTest scope;
        Log::Logger::setCapacity(0);
        Log::Logger::send(Log::Section{"ECS"}, Log::Level::Info, "first");
        Log::Logger::send(Log::Section{"ECS"}, Log::Level::Info, "second");
        const auto entries = Log::Logger::snapshot();
        expect((entries.size() == 1_u) >> fatal);
        expect(entries[0].message == "second");
    };

    "an impossible capacity throws and leaves the buffer unchanged"_test = [] {
        ScopedLoggerTest scope;
        Log::Logger::send(Log::Section{"ECS"}, Log::Level::Info, "kept");
        expect(throws([] { Log::Logger::setCapacity(std::numeric_limits<std::size_t>::max()); }));
        const auto entries = Log::Logger::snapshot();
        expect((entries.size() == 1_u) >> fatal);
        expect(entries[0].message == "kept");
    };
}
