#include <boost/ut.hpp>

#include <string>

#include "LoggerTestSupport.hpp"

using namespace boost::ut;

void run_logger_output_tests() {
    "Info goes to stdout in the exact format"_test = [] {
        ScopedLoggerTest scope;
        Log::Logger::send(Log::Section{"ECS"}, Log::Level::Info, "entity spawned");
        expect(scope.stdoutText() == "[Info][ECS] entity spawned\n");
        expect(scope.stderrText().empty());
    };

    "Warning and Error go to stderr"_test = [] {
        ScopedLoggerTest scope;
        Log::Logger::send(Log::Section{"Renderer"}, Log::Level::Warning, "texture missing");
        Log::Logger::send(Log::Section{"Renderer"}, Log::Level::Error, "crash");
        expect(scope.stderrText() == "[Warning][Renderer] texture missing\n[Error][Renderer] crash\n");
        expect(scope.stdoutText().empty());
    };

    "Debug and Success go to stdout"_test = [] {
        ScopedLoggerTest scope;
        Log::Logger::send(Log::Section{"ECS"}, Log::Level::Debug, "a");
        Log::Logger::send(Log::Section{"ECS"}, Log::Level::Success, "b");
        expect(scope.stdoutText() == "[Debug][ECS] a\n[Success][ECS] b\n");
        expect(scope.stderrText().empty());
    };

    "a filtered message prints nothing"_test = [] {
        ScopedLoggerTest scope;
        Log::Logger::setMinLevel(Log::Level::Error);
        Log::Logger::send(Log::Section{"ECS"}, Log::Level::Info, "hidden");
        Log::Logger::send(Log::Section{"ECS"}, Log::Level::Error, "shown");
        expect(scope.stdoutText().empty());
        expect(scope.stderrText() == "[Error][ECS] shown\n");
    };

    "an empty section prints as []"_test = [] {
        ScopedLoggerTest scope;
        Log::Logger::send(Log::Section{""}, Log::Level::Info, "hello");
        expect(scope.stdoutText() == "[Info][] hello\n");
    };

    "UTF-8, newline and tab print unchanged"_test = [] {
        ScopedLoggerTest scope;
        Log::Logger::send(Log::Section{"ECS"}, Log::Level::Info, "élément\n\tsuite");
        expect(scope.stdoutText() == "[Info][ECS] élément\n\tsuite\n");
    };

    "a 100 000 character message prints whole"_test = [] {
        ScopedLoggerTest scope;
        const std::string big(100000, 'x');
        Log::Logger::send(Log::Section{"ECS"}, Log::Level::Info, big);
        expect(scope.stdoutText() == "[Info][ECS] " + big + "\n");
    };
}
