#include "Core/Logger/Logger.hpp"

// The logger with logging off: what every build other than Debug gets.
// send and isEnabled are inline in Logger.hpp; these keep the rest of the
// API linking, and do nothing.
#ifdef ENGINE_LOG_ENABLED
#error "LoggerDisabled.cpp must not see ENGINE_LOG_ENABLED; compile Logger.cpp instead"
#endif

namespace Log {
    void Logger::setMinLevel(Level) {}
    void Logger::setCapacity(std::size_t) {}
    std::vector<Entry> Logger::snapshot() { return {}; }
    std::size_t Logger::droppedCount() { return 0; }
    void Logger::clear() {}
}
