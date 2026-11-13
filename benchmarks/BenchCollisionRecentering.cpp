// Why sweep(circle, rectangle) computes around (0, 0): precision and cost of
// building its rounded rectangle around the circle's start rather than in
// world space, at growing distances from the origin.
//
// Three variants, compiled alike (none inlined into the timing loop):
// - former:      the engine's code before this study: world space, and a whole
//                Hit copied for each of the six pieces.
// - world space: the current structure (only the first fraction kept), in
//                world space. The control: it differs from the engine's code
//                by the frame only.
// - around (0,0): Engine::Collision::sweep, the current code.
//
// Precision. For each distance D, the same 1,000,000 random cases (fixed
// seed), placed in a 20 m room centered on (D, D). The error of a hit is the
// distance between the two shapes at the reported fraction, computed in double
// from the same float inputs: 0 for a perfect answer. A wrong decision is a hit
// where the shapes never come within 10 µm (or start inside), no hit where they
// overlap by more than 10 µm, or a hit at a later contact than the first one.
//
// Cost. Nanoseconds per call on 4,096 prepared cases at 1 km. Paired: each
// repetition times the three variants back to back, in rotating order, so a
// change of CPU frequency during the run affects them alike; overheads are
// medians of the per-repetition ratios. Code placement changes timings too:
// build with -DBENCH_LAYOUT_PADDING=N (bytes, GCC/Clang) to shift the code and
// check that a result does not depend on it.
//
// Usage: bench_collision_recentering [repetitions] [all|precision|cost]
//        (defaults: 200, all)

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <optional>
#include <vector>

#include "BenchSupport.hpp"
#include "Engine/Common/Physic/Collision/Collision.hpp"

using namespace Engine::Collision;

#if defined(_MSC_VER)
    #define BENCH_NOINLINE __declspec(noinline)
#else
    #define BENCH_NOINLINE [[gnu::noinline]]
#endif

#ifndef BENCH_LAYOUT_PADDING
    #define BENCH_LAYOUT_PADDING 0
#endif

namespace {
#if BENCH_LAYOUT_PADDING > 0 && !defined(_MSC_VER)
    #define BENCH_STRING2(x) #x
    #define BENCH_STRING(x) BENCH_STRING2(x)
    // Placed before the variants: shifts their code by BENCH_LAYOUT_PADDING bytes.
    [[gnu::used, gnu::noinline]] void layoutPadding() { asm volatile(".skip " BENCH_STRING(BENCH_LAYOUT_PADDING) ", 0x90"); }
#endif

    // ------------------------------------------------------------------ the three variants

    BENCH_NOINLINE std::optional<Hit> sweepFormer(const Circle& moving, const Vector2f& delta, const Rectangle& target) {
        const float radius = moving.radius;
        const Vector2f& origin = moving.center;
        if (radius <= 0.f) {
            return raycast(origin, delta, target);
        }
        if ((origin - target.closestPoint(origin)).lengthSquared() < radius * radius) {
            return std::nullopt;
        }
        std::optional<Hit> first;
        const auto keepFirst = [&first](const std::optional<Hit>& hit) {
            if (hit && (!first || hit->fraction < first->fraction)) {
                first = hit;
            }
        };
        keepFirst(raycast(origin, delta, Rectangle{{target.min.x - radius, target.min.y}, {target.max.x + radius, target.max.y}}));
        keepFirst(raycast(origin, delta, Rectangle{{target.min.x, target.min.y - radius}, {target.max.x, target.max.y + radius}}));
        keepFirst(raycast(origin, delta, Circle{target.min, radius}));
        keepFirst(raycast(origin, delta, Circle{{target.max.x, target.min.y}, radius}));
        keepFirst(raycast(origin, delta, Circle{{target.min.x, target.max.y}, radius}));
        keepFirst(raycast(origin, delta, Circle{target.max, radius}));
        if (!first) {
            return std::nullopt;
        }
        const Vector2f outward = first->point - target.closestPoint(first->point);
        if (const float length = outward.length(); length > 0.f) {
            first->normal = {outward.x / length, outward.y / length};
        }
        return first;
    }

    BENCH_NOINLINE std::optional<Hit> sweepInWorldSpace(const Circle& moving, const Vector2f& delta, const Rectangle& target) {
        const float radius = moving.radius;
        const Vector2f& origin = moving.center;
        if (radius <= 0.f) {
            return raycast(origin, delta, target);
        }
        if ((origin - target.closestPoint(origin)).lengthSquared() < radius * radius) {
            return std::nullopt;
        }
        float fraction = 2.f;
        const auto keepFirst = [&fraction](const std::optional<Hit>& hit) {
            if (hit && hit->fraction < fraction) {
                fraction = hit->fraction;
            }
        };
        keepFirst(raycast(origin, delta, Rectangle{{target.min.x - radius, target.min.y}, {target.max.x + radius, target.max.y}}));
        keepFirst(raycast(origin, delta, Rectangle{{target.min.x, target.min.y - radius}, {target.max.x, target.max.y + radius}}));
        keepFirst(raycast(origin, delta, Circle{target.min, radius}));
        keepFirst(raycast(origin, delta, Circle{{target.max.x, target.min.y}, radius}));
        keepFirst(raycast(origin, delta, Circle{{target.min.x, target.max.y}, radius}));
        keepFirst(raycast(origin, delta, Circle{target.max, radius}));
        if (fraction > 1.f) {
            return std::nullopt;
        }
        const Vector2f center = origin + delta * fraction;
        const Vector2f outward = center - target.closestPoint(center);
        const float length = outward.length();
        const Vector2f normal = length > 0.f ? Vector2f(outward.x / length, outward.y / length)
                                             : Vector2f(-delta.x / delta.length(), -delta.y / delta.length());
        return Hit{fraction, center, normal};
    }

    BENCH_NOINLINE std::optional<Hit> sweepAroundOrigin(const Circle& moving, const Vector2f& delta, const Rectangle& target) {
        return sweep(moving, delta, target);
    }

    // ------------------------------------------------------------------ oracle, in double

    constexpr double Band = 1e-5;  // 10 µm: closer than this to touching, either answer is right

    struct Case {
        Circle moving;
        Vector2f delta;
        Rectangle target;
    };

    // Distance from the moving circle to the rectangle once the motion is done up to t.
    double distanceAt(const Case& c, double t) {
        const double x = double(c.moving.center.x) + double(c.delta.x) * t;
        const double y = double(c.moving.center.y) + double(c.delta.y) * t;
        const double dx = std::max(double(c.target.min.x) - x, x - double(c.target.max.x));
        const double dy = std::max(double(c.target.min.y) - y, y - double(c.target.max.y));
        return std::hypot(std::max(dx, 0.0), std::max(dy, 0.0)) + std::min(std::max(dx, dy), 0.0) - double(c.moving.radius);
    }

    // The distance along the motion is convex in t: ternary search finds its minimum.
    double closestOver(const Case& c, double low, double high) {
        for (int i = 0; i < 60; ++i) {
            const double a = low + (high - low) / 3.0;
            const double b = high - (high - low) / 3.0;
            if (distanceAt(c, a) < distanceAt(c, b)) {
                high = b;
            } else {
                low = a;
            }
        }
        return std::min({distanceAt(c, low), distanceAt(c, high), distanceAt(c, (low + high) / 2)});
    }

    // ------------------------------------------------------------------ scenario

    // splitmix64: the same cases on every platform.
    struct Random {
        std::uint64_t state;
        double uniform(double low, double high) {
            std::uint64_t z = (state += 0x9E3779B97F4A7C15ULL);
            z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
            z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
            z ^= z >> 31;
            return low + (high - low) * double(z >> 11) * 0x1.0p-53;
        }
    };

    // A rectangle (a tile to a platform) in a 20 m room centered on (distance, distance),
    // and a circle (a character, a projectile) within 6 m of it, moving up to 6 m per axis.
    // Built in double, rounded once to float: the same scene at every distance.
    std::vector<Case> makeCases(double distance, int count, std::uint64_t seed) {
        Random random{seed};
        std::vector<Case> cases;
        cases.reserve(static_cast<std::size_t>(count));
        const auto f = [](double v) { return static_cast<float>(v); };
        for (int i = 0; i < count; ++i) {
            const double cx = random.uniform(-10, 10), cy = random.uniform(-10, 10);
            const double hx = random.uniform(0.1, 3), hy = random.uniform(0.1, 3);
            const double sx = cx + random.uniform(-6, 6), sy = cy + random.uniform(-6, 6);
            const double radius = random.uniform(0.1, 1.5);
            const double mx = random.uniform(-6, 6), my = random.uniform(-6, 6);
            cases.push_back({Circle{{f(distance + sx), f(distance + sy)}, f(radius)},
                             Vector2f{f(mx), f(my)},
                             Rectangle{{f(distance + cx - hx), f(distance + cy - hy)}, {f(distance + cx + hx), f(distance + cy + hy)}}});
        }
        return cases;
    }

    // ------------------------------------------------------------------ precision

    struct Precision {
        long hits = 0;
        long beyondBand = 0;  // hits whose contact is off by more than 10 µm
        long wrongDecisions = 0;
        double median = 0, p99 = 0, worst = 0;  // error of the contact, metres
        double worstNormal = 0;                  // |normal - true normal| on clear hits ≈ radians
    };

    template <typename Sweep>
    Precision measurePrecision(const std::vector<Case>& cases, const std::vector<double>& atStart,
                               const std::vector<double>& closest, Sweep sweepFunction) {
        Precision p;
        std::vector<double> errors;
        for (std::size_t i = 0; i < cases.size(); ++i) {
            const Case& c = cases[i];
            const std::optional<Hit> hit = sweepFunction(c.moving, c.delta, c.target);
            const bool startsInside = atStart[i] < -Band;
            const bool neverTouches = atStart[i] > Band && closest[i] > Band;
            const bool clearlyGoesIn = atStart[i] > Band && closest[i] < -Band;
            if (!hit) {
                p.wrongDecisions += clearlyGoesIn ? 1 : 0;
                continue;
            }
            ++p.hits;
            const double t = hit->fraction;
            const double error = std::fabs(distanceAt(c, t));
            errors.push_back(error);
            p.beyondBand += error > Band ? 1 : 0;
            // A later contact than the first: they overlapped before, deeper than the error at the hit explains.
            const bool laterContact = closestOver(c, 0.0, t) < -(Band + error);
            if (startsInside || neverTouches || laterContact) {
                ++p.wrongDecisions;
            }
            if (clearlyGoesIn) {  // true normal: from the rectangle's closest point to the center at impact
                const double x = double(c.moving.center.x) + double(c.delta.x) * t;
                const double y = double(c.moving.center.y) + double(c.delta.y) * t;
                const double ox = x - std::clamp(x, double(c.target.min.x), double(c.target.max.x));
                const double oy = y - std::clamp(y, double(c.target.min.y), double(c.target.max.y));
                const double length = std::hypot(ox, oy);
                if (length > 0) {
                    p.worstNormal = std::max(p.worstNormal, std::hypot(hit->normal.x - ox / length, hit->normal.y - oy / length));
                }
            }
        }
        std::sort(errors.begin(), errors.end());
        if (!errors.empty()) {
            p.median = errors[errors.size() / 2];
            p.p99 = errors[errors.size() * 99 / 100];
            p.worst = errors.back();
        }
        return p;
    }

    // ------------------------------------------------------------------ cost

    volatile double g_sink = 0;  // keeps every output of every call alive

    using SweepFunction = std::optional<Hit> (*)(const Circle&, const Vector2f&, const Rectangle&);

    // Nanoseconds per call over `passes` passes on the cases; counts the hits.
    double timeCalls(const std::vector<Case>& cases, int passes, SweepFunction sweepFunction, std::size_t& hits) {
        double checksum = 0;
        hits = 0;
        const auto start = std::chrono::steady_clock::now();
        for (int pass = 0; pass < passes; ++pass) {
            for (const Case& c : cases) {
                if (const std::optional<Hit> hit = sweepFunction(c.moving, c.delta, c.target)) {
                    ++hits;
                    checksum += hit->fraction + hit->point.x + hit->normal.y;
                }
            }
        }
        const auto end = std::chrono::steady_clock::now();
        g_sink = checksum;
        return std::chrono::duration<double, std::nano>(end - start).count() / (double(cases.size()) * passes);
    }

    double quantile(std::vector<double> values, double q) {
        std::sort(values.begin(), values.end());
        return values[static_cast<std::size_t>(q * double(values.size() - 1))];
    }

    struct Cost {
        double former = 0, world = 0, local = 0;  // median ns per call
        double recentering = 0, recenteringP25 = 0, recenteringP75 = 0;  // around (0,0) vs world space, percent
        double versusFormer = 0, versusFormerP25 = 0, versusFormerP75 = 0;  // around (0,0) vs former, percent
        bool sameHits = true;
    };

    Cost measureCost(const std::vector<Case>& cases, int passes, int repetitions) {
        const SweepFunction variants[3] = {sweepFormer, sweepInWorldSpace, sweepAroundOrigin};
        std::size_t hits[3] = {};
        for (int i = 0; i < std::max(1, repetitions / 10); ++i) {  // warm-up
            for (int v = 0; v < 3; ++v) {
                timeCalls(cases, passes, variants[v], hits[v]);
            }
        }
        std::vector<double> times[3], recentering, versusFormer;
        for (int r = 0; r < repetitions; ++r) {
            double t[3] = {};
            for (int k = 0; k < 3; ++k) {  // rotating order: each variant runs first, second and third alike
                const int v = (r + k) % 3;
                t[v] = timeCalls(cases, passes, variants[v], hits[v]);
            }
            for (int v = 0; v < 3; ++v) {
                times[v].push_back(t[v]);
            }
            recentering.push_back((t[2] / t[1] - 1.0) * 100.0);
            versusFormer.push_back((t[2] / t[0] - 1.0) * 100.0);
        }
        return {quantile(times[0], 0.5), quantile(times[1], 0.5), quantile(times[2], 0.5),
                quantile(recentering, 0.5), quantile(recentering, 0.25), quantile(recentering, 0.75),
                quantile(versusFormer, 0.5), quantile(versusFormer, 0.25), quantile(versusFormer, 0.75),
                hits[0] == hits[1] && hits[1] == hits[2]};
    }
}

int main(int argc, char** argv) {
#if defined(__clang__)
    std::printf("compiler: clang %s\n", __clang_version__);
#elif defined(__GNUC__)
    std::printf("compiler: gcc %s\n", __VERSION__);
#elif defined(_MSC_VER)
    std::printf("compiler: msvc %d\n", _MSC_FULL_VER);
#endif
#ifndef NDEBUG
    std::printf("WARNING: not a Release build, the timings mean nothing\n");
#endif
    std::printf("layout padding: %d bytes\n", BENCH_LAYOUT_PADDING);
    const int repetitions = Bench::repetitionsFromArgs(argc, argv);
    const char* mode = argc > 2 ? argv[2] : "all";
    const bool runPrecision = std::strcmp(mode, "cost") != 0;
    const bool runCost = std::strcmp(mode, "precision") != 0;
    constexpr std::uint64_t Seed = 20261110;

    // ---- precision
    constexpr int CasesPerDistance = 1'000'000;
    constexpr double Distances[] = {0.0, 100.0, 1'000.0, 10'000.0, 100'000.0};
    double worstLocal = 0, worstAtZero = 0, worstAtFarthest = 0;
    long wrongLocal = 0;
    if (runPrecision) {
        std::printf("\nprecision: %d cases per distance, seed %llu, band %.0e m\n", CasesPerDistance,
                    static_cast<unsigned long long>(Seed), Band);
        std::printf("%-10s %-11s %-12s %8s %11s %11s %11s %13s %9s %9s\n", "distance", "float step", "variant", "hits",
                    "median (m)", "p99 (m)", "max (m)", "normal (rad)", "> 10 um", "decision");
        for (const double distance : Distances) {
            const std::vector<Case> cases = makeCases(distance, CasesPerDistance, Seed);
            std::vector<double> atStart(cases.size()), closest(cases.size());
            for (std::size_t i = 0; i < cases.size(); ++i) {
                atStart[i] = distanceAt(cases[i], 0.0);
                closest[i] = closestOver(cases[i], 0.0, 1.0);
            }
            const double step = std::ldexp(1.0, std::ilogb(std::max(distance, 1.0)) - 23);
            const Precision world = measurePrecision(cases, atStart, closest, sweepInWorldSpace);
            const Precision local = measurePrecision(cases, atStart, closest, sweepAroundOrigin);
            for (const auto& [name, p] : {std::pair{"world space", world}, std::pair{"around (0,0)", local}}) {
                std::printf("%-10.0f %-11.2e %-12s %8ld %11.2e %11.2e %11.2e %13.2e %9ld %9ld\n", distance, step, name,
                            p.hits, p.median, p.p99, p.worst, p.worstNormal, p.beyondBand, p.wrongDecisions);
            }
            worstLocal = std::max(worstLocal, local.worst);
            wrongLocal += local.wrongDecisions + local.beyondBand;
            if (distance == Distances[0]) {
                worstAtZero = local.worst;
            }
            worstAtFarthest = local.worst;
        }
    }

    // ---- cost
    constexpr int CostCases = 4096;
    constexpr int Passes = 16;
    constexpr double SweepsPerFrame = 2000;      // a busy room: 100 moving circles x 20 nearby rectangles
    constexpr double FrameNs = 1e9 / 60.0;       // 16.67 ms at 60 Hz
    constexpr double BudgetNs = FrameNs * 0.01;  // 1 % of the frame
    double recenteringNs = 0;
    if (runCost) {
        const std::vector<Case> costCases = makeCases(1'000.0, CostCases, Seed + 1);
        std::printf("\ncost: %d cases at 1 km x %d passes per timing, %d paired repetitions\n", CostCases, Passes, repetitions);
        const Cost cost = measureCost(costCases, Passes, repetitions);
        if (!cost.sameHits) {
            std::printf("WARNING: the variants disagree on the number of hits\n");
        }
        std::printf("%-14s median %6.2f ns/call\n", "former", cost.former);
        std::printf("%-14s median %6.2f ns/call\n", "world space", cost.world);
        std::printf("%-14s median %6.2f ns/call\n", "around (0,0)", cost.local);
        std::printf("recentering alone (around (0,0) vs world space): %+.1f %% (quartiles %+.1f %% / %+.1f %%)\n",
                    cost.recentering, cost.recenteringP25, cost.recenteringP75);
        std::printf("current code vs former code:                     %+.1f %% (quartiles %+.1f %% / %+.1f %%)\n",
                    cost.versusFormer, cost.versusFormerP25, cost.versusFormerP75);
        recenteringNs = cost.world * cost.recentering / 100.0;
        std::printf("recentering per frame (%.0f sweeps): %+.1f us, %.3f %% of a 60 Hz frame\n", SweepsPerFrame,
                    recenteringNs * SweepsPerFrame / 1000.0, recenteringNs * SweepsPerFrame / FrameNs * 100.0);
    }

    // ---- success criteria (for the current code, around (0, 0))
    std::printf("\ncriteria\n");
    if (runPrecision) {
        std::printf("  C1 max error <= 1e-5 m at every distance           %.2e m   %s\n", worstLocal,
                    worstLocal <= 1e-5 ? "PASS" : "FAIL");
        std::printf("  C2 max error at 100 km <= 2 x at 0 m               %.2f x     %s\n", worstAtFarthest / worstAtZero,
                    worstAtFarthest <= 2 * worstAtZero ? "PASS" : "FAIL");
        std::printf("  C3 no wrong decision, no contact off by > 10 um    %ld         %s\n", wrongLocal,
                    wrongLocal == 0 ? "PASS" : "FAIL");
    }
    if (runCost) {
        std::printf("  C4 recentering x %.0f sweeps <= 1 %% of a frame     %+.1f us  %s\n", SweepsPerFrame,
                    recenteringNs * SweepsPerFrame / 1000.0, recenteringNs * SweepsPerFrame <= BudgetNs ? "PASS" : "FAIL");
    }
    return 0;
}
