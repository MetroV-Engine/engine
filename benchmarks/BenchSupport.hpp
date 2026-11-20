// Shared helpers for the wall-clock benchmarks in this directory.

#pragma once

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
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

    namespace Detail {
        struct NamedResult {
            std::string name;
            Result result;
        };

        // Filled by report(); written by writeJson() when --json was given.
        struct Run {
            std::string benchmark;
            std::string jsonPath;
            int repetitions = 0;
            std::vector<NamedResult> results;
        };

        inline Run& run() {
            static Run instance;
            return instance;
        }

        inline void writeEscaped(std::FILE* file, const std::string& text) {
            std::fputc('"', file);
            for (const char c : text) {
                if (c == '"' || c == '\\') {
                    std::fputc('\\', file);
                    std::fputc(c, file);
                } else if (static_cast<unsigned char>(c) < 0x20) {
                    std::fprintf(file, "\\u%04x", static_cast<unsigned>(c));
                } else {
                    std::fputc(c, file);
                }
            }
            std::fputc('"', file);
        }

        // Registered with atexit, so it runs when main returns.
        inline void writeJson() {
            const Run& current = run();
            std::FILE* file = std::fopen(current.jsonPath.c_str(), "w");
            if (file == nullptr) {
                std::fprintf(stderr, "cannot write %s\n", current.jsonPath.c_str());
                std::_Exit(EXIT_FAILURE); // so CI notices the missing results
            }
            std::fputs("{\n  \"benchmark\": ", file);
            writeEscaped(file, current.benchmark);
            std::fprintf(file, ",\n  \"repetitions\": %d,\n  \"results\": [", current.repetitions);
            for (std::size_t i = 0; i < current.results.size(); ++i) {
                const NamedResult& entry = current.results[i];
                std::fputs(i == 0 ? "\n    {\"name\": " : ",\n    {\"name\": ", file);
                writeEscaped(file, entry.name);
                std::fprintf(file, ", \"median_us\": %.3f, \"min_us\": %.3f, \"matched\": %zu}",
                             entry.result.medianMicros, entry.result.minMicros,
                             entry.result.matched);
            }
            std::fputs("\n  ]\n}\n", file);
            std::fclose(file);
        }
    }

    inline void report(const char* name, const Result& result) {
        std::printf("%-56s median %10.2f us   min %10.2f us   matched %zu\n",
                    name, result.medianMicros, result.minMicros, result.matched);
        Detail::run().results.push_back({name, result});
    }

    // Reads the command line: [repetitions] [--json <file>].
    // repetitions defaults to 200. With --json, every report() is also written
    // to <file> as JSON when the program exits (used by CI, see README.md).
    inline int repetitionsFromArgs(int argc, char** argv) {
        Detail::Run& current = Detail::run();
        int repetitions = 200;
        for (int i = 1; i < argc; ++i) {
            if (std::strcmp(argv[i], "--json") == 0 && i + 1 < argc) {
                current.jsonPath = argv[++i];
            } else {
                repetitions = std::max(1, std::atoi(argv[i]));
            }
        }

        // Benchmark name: the executable name without its directory.
        std::string name = argc > 0 ? argv[0] : "benchmark";
        const auto slash = name.find_last_of("/\\");
        if (slash != std::string::npos) {
            name.erase(0, slash + 1);
        }
        if (name.size() > 4 && name.compare(name.size() - 4, 4, ".exe") == 0) {
            name.erase(name.size() - 4);
        }
        current.benchmark = name;
        current.repetitions = repetitions;
        if (!current.jsonPath.empty()) {
            std::atexit(Detail::writeJson);
        }

        std::printf("repetitions per scenario: %d\n", repetitions);
        return repetitions;
    }
}
