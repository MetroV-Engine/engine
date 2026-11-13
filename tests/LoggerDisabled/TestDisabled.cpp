#include <boost/ut.hpp>

#include <string>

#include "../Logger/LoggerTestSupport.hpp"

using namespace boost::ut;

namespace {
    int evaluations = 0;

    std::string countedName() {
        ++evaluations;
        return "ECS";
    }

    Log::Level countedLevel() {
        ++evaluations;
        return Log::Level::Error;
    }

    std::string counted(std::string text) {
        ++evaluations;
        return text;
    }
}

void run_logger_disabled_tests() {
    "ENGINE_LOG evaluates none of its arguments"_test = [] {
        ScopedLoggerTest scope;
        evaluations = 0;
        ENGINE_LOG(Log::Section{countedName()}, countedLevel(), counted("never built"));
        expect(evaluations == 0_i);
    };

    "send stores, drops and prints nothing"_test = [] {
        ScopedLoggerTest scope;
        for (int i = 0; i < 1030; ++i) {
            Log::Logger::send(Log::Section{"ECS"}, Log::Level::Error, "should not appear");
        }
        expect(Log::Logger::snapshot().empty());
        expect(Log::Logger::droppedCount() == 0_u);
        expect(scope.stdoutText().empty());
        expect(scope.stderrText().empty());
    };

    "isEnabled is always false"_test = [] {
        ScopedLoggerTest scope;
        expect(!Log::Logger::isEnabled(Log::Level::Error));
    };
}
