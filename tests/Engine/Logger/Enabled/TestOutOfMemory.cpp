#include <boost/ut.hpp>

#include <atomic>
#include <cstdlib>
#include <new>
#include <string>

#include "LoggerTestSupport.hpp"

using namespace boost::ut;

// Replaces the global allocator for the whole tests_logger program. It behaves
// like the default one until a test sets failNextAllocation: the next
// allocation then throws std::bad_alloc, once.
namespace {
    std::atomic<bool> failNextAllocation{false};
}

void* operator new(std::size_t size) {
    if (failNextAllocation.exchange(false)) {
        throw std::bad_alloc();
    }
    if (void* memory = std::malloc(size == 0 ? 1 : size)) {
        return memory;
    }
    throw std::bad_alloc();
}

void operator delete(void* memory) noexcept {
    std::free(memory);
}

void operator delete(void* memory, std::size_t) noexcept {
    std::free(memory);
}

void run_logger_out_of_memory_tests() {
    "send does not throw and leaves the buffer unchanged when memory runs out"_test = [] {
        ScopedLoggerTest scope;
        Log::Logger::send(Log::Section{"ECS"}, Log::Level::Info, "a");
        Log::Logger::send(Log::Section{"ECS"}, Log::Level::Info, "b");
        const std::string longText(200, 'z');  // too long for the small-string buffer, so copying it allocates

        const bool sendReturned = static_cast<bool>(nothrow([&] {
            failNextAllocation = true;  // set here so Boost.UT's own allocations never take the failure
            Log::Logger::send(Log::Section{"ECS"}, Log::Level::Info, longText);
        }));
        failNextAllocation = false;

        expect(sendReturned);
        const auto entries = Log::Logger::snapshot();
        expect((entries.size() == 2_u) >> fatal);
        expect(entries[0].message == "a");
        expect(entries[1].message == "b");
        expect(Log::Logger::droppedCount() == 0_u);
    };
}
