#pragma once

#include <exception>
#include <functional>
#include <iostream>
#include <string>

namespace ecs_test {
    inline int& failure_count() {
        static int count = 0;
        return count;
    }

    inline void check(bool cond, const char* expr, const char* file, int line) {
        if (!cond) {
            std::cerr << "  CHECK FAILED: " << expr << " (" << file << ":" << line << ")\n";
            ++failure_count();
        }
    }

    inline void run(const std::string& name, const std::function<void()>& fn) {
        int before = failure_count();
        try {
            fn();
        } catch (const std::exception& e) {
            std::cerr << "  UNCAUGHT EXCEPTION: " << e.what() << "\n";
            ++failure_count();
        } catch (...) {
            std::cerr << "  UNCAUGHT EXCEPTION (non-std)\n";
            ++failure_count();
        }
        std::cout << (failure_count() == before ? "[PASS] " : "[FAIL] ") << name << "\n";
    }
}

#define CHECK(expr) ecs_test::check((expr), #expr, __FILE__, __LINE__)
