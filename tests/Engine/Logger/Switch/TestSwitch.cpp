#include <boost/ut.hpp>

#include "Engine/Logger/Logger.hpp"

using namespace boost::ut;

// ENGINE_EXPECT_LOG_ENABLED comes from CMake: 1 for a Debug build, 0 otherwise.
#ifdef ENGINE_LOG_ENABLED
constexpr bool logEnabled = true;
#else
constexpr bool logEnabled = false;
#endif

int main()
{
    "linking engine_logger enables logging in Debug builds only"_test = [] {
        expect(logEnabled == (ENGINE_EXPECT_LOG_ENABLED == 1));
    };
}
