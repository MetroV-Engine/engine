#include <boost/ut.hpp>

#include <iostream>
#include <stdexcept>
#include <streambuf>

#include "LoggerTestSupport.hpp"

using namespace boost::ut;

namespace {
    // A stream buffer whose every write fails.
    struct FailingBuffer : std::streambuf {
        int_type overflow(int_type) override { throw std::runtime_error("write failed"); }
        std::streamsize xsputn(const char*, std::streamsize) override { throw std::runtime_error("write failed"); }
    };
}

void run_logger_write_failure_tests() {
    "send does not throw when stdout fails, and the entry stays stored"_test = [] {
        ScopedLoggerTest scope;
        FailingBuffer failing;
        std::streambuf* previous = std::cout.rdbuf(&failing);
        std::cout.exceptions(std::ios::badbit);

        const bool sendReturned = static_cast<bool>(nothrow([] {
            Log::Logger::send(Log::Section{"ECS"}, Log::Level::Info, "lost line");
        }));

        std::cout.exceptions(std::ios::goodbit);
        std::cout.clear();
        std::cout.rdbuf(previous);

        expect(sendReturned);
        const auto entries = Log::Logger::snapshot();
        expect((entries.size() == 1_u) >> fatal);
        expect(entries[0].message == "lost line");
    };
}
