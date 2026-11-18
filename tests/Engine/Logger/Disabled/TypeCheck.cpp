// Compiled with logging off, twice. With LOGGER_VALID_ARGUMENT the call is
// correct and must build. Without it, the message is an int, which Debug
// rejects, so Release must reject it too: ENGINE_LOG type-checks its arguments
// in every build even though it evaluates nothing.

#include "Engine/Logger/Logger.hpp"

inline constexpr Log::Section ECS{"ECS"};

int main()
{
#ifdef LOGGER_VALID_ARGUMENT
    ENGINE_LOG(ECS, Log::Level::Info, "text");
#else
    ENGINE_LOG(ECS, Log::Level::Info, 42);
#endif
    return 0;
}
