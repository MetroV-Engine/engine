// Shared helpers for the wall-clock benchmarks in this directory.

#pragma once

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <vector>

namespace Bench {
    struct Result {
        double medianMicros;
        double minMicros;
        std::size_t matched;
    };

    // Runs query a few times to warm up, then times it `repetitions` times.
    // query must return a count, reported as "matched" to sanity-check runs.
    template<typename Query>
    Result measure(Query&& query, int repetitions) {
        const int warmup = std::max(1, repetitions / 10);
        std::size_t matched = 0;
        for (int i = 0; i < warmup; ++i) {
            matched = query();
        }

        std::vector<double> samples;
        samples.reserve(static_cast<std::size_t>(repetitions));
        for (int i = 0; i < repetitions; ++i) {
            const auto start = std::chrono::steady_clock::now();
            matched = query();
            const auto end = std::chrono::steady_clock::now();
            samples.push_back(
                std::chrono::duration<double, std::micro>(end - start).count());
        }

        std::sort(samples.begin(), samples.end());
        return {samples[samples.size() / 2], samples.front(), matched};
    }

    inline void report(const char* name, const Result& result) {
        std::printf("%-56s median %10.2f us   min %10.2f us   matched %zu\n",
                    name, result.medianMicros, result.minMicros, result.matched);
    }

    // First argument is the repetition count; defaults to 200.
    inline int repetitionsFromArgs(int argc, char** argv) {
        const int repetitions = argc > 1 ? std::max(1, std::atoi(argv[1])) : 200;
        std::printf("repetitions per scenario: %d\n", repetitions);
        return repetitions;
    }
}
