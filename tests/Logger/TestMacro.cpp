#include <boost/ut.hpp>

#include <string>
#include <string_view>

#include "LoggerTestSupport.hpp"

using namespace boost::ut;

namespace {
    constexpr Log::Section ECS{"ECS"};

    int evaluations = 0;

    std::string counted(std::string text) {
        ++evaluations;
        return text;
    }
}

void run_logger_macro_tests() {
    "ENGINE_LOG stores a message"_test = [] {
        ScopedLoggerTest scope;
        ENGINE_LOG(ECS, Log::Level::Info, "via macro");
        const auto entries = Log::Logger::snapshot();
        expect((entries.size() == 1_u) >> fatal);
        expect(entries[0].section == "ECS");
        expect(entries[0].message == "via macro");
    };

    "ENGINE_LOG builds the message only when its level passes the filter"_test = [] {
        ScopedLoggerTest scope;
        evaluations = 0;
        Log::Logger::setMinLevel(Log::Level::Warning);
        ENGINE_LOG(ECS, Log::Level::Info, counted("filtered"));
        expect(evaluations == 0_i);
        ENGINE_LOG(ECS, Log::Level::Warning, counted("kept"));
        expect(evaluations == 1_i);
        const auto entries = Log::Logger::snapshot();
        expect((entries.size() == 1_u) >> fatal);
        expect(entries[0].message == "kept");
    };

    "ENGINE_LOG evaluates its level once"_test = [] {
        ScopedLoggerTest scope;
        int levelCalls = 0;
        auto level = [&] {
            ++levelCalls;
            return Log::Level::Info;
        };
        ENGINE_LOG(ECS, level(), "m");
        expect(levelCalls == 1_i);
        expect(Log::Logger::snapshot().size() == 1_u);
    };

    "ENGINE_LOG accepts std::string and std::string_view messages"_test = [] {
        ScopedLoggerTest scope;
        const std::string owned = "owned text";
        const std::string_view view = "viewed text";
        ENGINE_LOG(ECS, Log::Level::Info, owned);
        ENGINE_LOG(ECS, Log::Level::Info, view);
        const auto entries = Log::Logger::snapshot();
        expect((entries.size() == 2_u) >> fatal);
        expect(entries[0].message == "owned text");
        expect(entries[1].message == "viewed text");
    };

    "ENGINE_LOG copies a section built from a temporary string"_test = [] {
        ScopedLoggerTest scope;
        ENGINE_LOG(Log::Section{std::string("Temporary")}, Log::Level::Info, "m");
        const auto entries = Log::Logger::snapshot();
        expect((entries.size() == 1_u) >> fatal);
        expect(entries[0].section == "Temporary");
    };

    "ENGINE_LOG works as an if body without braces"_test = [] {
        ScopedLoggerTest scope;
        const bool takeThen = false;
        if (takeThen)
            ENGINE_LOG(ECS, Log::Level::Info, "then");
        else
            ENGINE_LOG(ECS, Log::Level::Info, "else");
        const auto entries = Log::Logger::snapshot();
        expect((entries.size() == 1_u) >> fatal);
        expect(entries[0].message == "else");
    };
}
