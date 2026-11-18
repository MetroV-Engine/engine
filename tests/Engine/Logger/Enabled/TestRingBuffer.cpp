#include <boost/ut.hpp>

#include <string>

#include "LoggerTestSupport.hpp"

using namespace boost::ut;

namespace {
    // Sends the messages "first" .. "first + count - 1".
    void sendNumbered(int first, int count) {
        for (int i = first; i < first + count; ++i) {
            Log::Logger::send(Log::Section{"ECS"}, Log::Level::Info, std::to_string(i));
        }
    }
}

void run_logger_ring_buffer_tests() {
    "the buffer keeps the newest 1024 of 1030 messages"_test = [] {
        ScopedLoggerTest scope;
        sendNumbered(0, 1030);
        const auto entries = Log::Logger::snapshot();
        expect((entries.size() == 1024_u) >> fatal);
        expect(entries.front().message == "6");
        expect(entries.back().message == "1029");
        expect(Log::Logger::droppedCount() == 6_u);
    };

    "snapshot is oldest first after a wrap"_test = [] {
        ScopedLoggerTest scope;
        sendNumbered(0, 1030);
        const auto entries = Log::Logger::snapshot();
        expect((entries.size() == 1024_u) >> fatal);
        expect(entries[0].message == "6");
        expect(entries[1].message == "7");
        expect(entries[511].message == "517");
        expect(entries[1022].message == "1028");
    };

    "the 1025th message is the first one dropped"_test = [] {
        ScopedLoggerTest scope;
        sendNumbered(0, 1024);
        expect(Log::Logger::droppedCount() == 0_u);
        expect(Log::Logger::snapshot().front().message == "0");
        sendNumbered(1024, 1);
        expect(Log::Logger::droppedCount() == 1_u);
        expect(Log::Logger::snapshot().front().message == "1");
    };

    "clear resets the dropped count"_test = [] {
        ScopedLoggerTest scope;
        sendNumbered(0, 1030);
        expect((Log::Logger::droppedCount() == 6_u) >> fatal);
        Log::Logger::clear();
        expect(Log::Logger::droppedCount() == 0_u);
        expect(Log::Logger::snapshot().empty());
    };
}
