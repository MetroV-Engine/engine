#pragma once

#include <iostream>
#include <sstream>
#include <streambuf>
#include <string>

#include "Core/Logger/Logger.hpp"

// Gives a test a fresh logger and captures what it prints. Both are undone
// when the object goes out of scope, so tests never see each other's state
// and a test run prints nothing but results.
class ScopedLoggerTest {
public:
    ScopedLoggerTest()
        : _oldOut(std::cout.rdbuf(_out.rdbuf())), _oldErr(std::cerr.rdbuf(_err.rdbuf())) {
        reset();
    }

    ~ScopedLoggerTest() {
        std::cout.rdbuf(_oldOut);
        std::cerr.rdbuf(_oldErr);
        reset();
    }

    ScopedLoggerTest(const ScopedLoggerTest&) = delete;
    ScopedLoggerTest& operator=(const ScopedLoggerTest&) = delete;

    std::string stdoutText() const { return _out.str(); }
    std::string stderrText() const { return _err.str(); }

private:
    static void reset() {
        Log::Logger::setMinLevel(Log::Level::Debug);
        Log::Logger::clear();
    }

    std::ostringstream _out;
    std::ostringstream _err;
    std::streambuf* _oldOut;
    std::streambuf* _oldErr;
};
