// Logs from a global destructor, after main has returned. CTest runs this
// program under Valgrind: if the logger's state is gone by then, the late
// call reads and writes freed memory and Valgrind fails the test.

#include "Engine/Logger/Logger.hpp"

namespace {
    struct LogsOnDestruction {
        ~LogsOnDestruction() {
            ENGINE_LOG(Log::Section{"ECS"}, Log::Level::Info, "bye from a global destructor");
        }
    };

    // Constructed before main, so it is destroyed after anything main creates.
    LogsOnDestruction lateLogger;
}

int main()
{
    // The first message creates the logger's state, after lateLogger exists.
    ENGINE_LOG(Log::Section{"ECS"}, Log::Level::Info, "first message creates the logger state");
    return 0;
}
