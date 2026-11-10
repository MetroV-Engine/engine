#pragma once

/**
 * @file
 * @brief Random checks of the collision geometry against an independent oracle.
 *
 * The oracle is the signed distance between two shapes, computed in double:
 * how far apart they are, negative when they overlap (minus how deep). Every
 * collision query makes a statement about it, which random cases can check:
 * - overlaps(a, b) is true exactly when the distance is <= 0.
 * - collide(a, b): depth is minus the distance, the shortest push that
 *   separates the shapes; pushing by normal * depth leaves them touching.
 * - raycast and sweep: along the motion, the distance f(t) is a convex
 *   function of t (the distance to a convex shape, along a straight line).
 *   Its minimum is found by ternary search, and the first contact is the
 *   first root of f.
 *
 * Within tolerance() of 0 (shapes touching, a ray grazing a surface), float
 * rounding cannot decide, and either answer is accepted.
 */

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <optional>
#include <string>

#include "Engine/Common/Collision/Collision.hpp"
#include "Engine/Common/Math/Vector.hpp"

namespace TestSupport {
    using Engine::Collision::Circle;
    using Engine::Collision::Contact;
    using Engine::Collision::Hit;
    using Engine::Collision::Rectangle;
    using Engine::Math::Vector2d;
    using Engine::Math::Vector2f;

    // ------------------------------------------------------------------ exact shapes

    /** @brief A rectangle in double, for the oracle. */
    struct ExactRectangle {
        Vector2d min;
        Vector2d max;
    };

    /** @brief A circle in double, for the oracle. */
    struct ExactCircle {
        Vector2d center;
        double radius = 0.0;
    };

    [[nodiscard]] inline Vector2d exact(const Vector2f& vector) { return Vector2d(vector); }
    [[nodiscard]] inline ExactRectangle exact(const Rectangle& rectangle) { return {exact(rectangle.min), exact(rectangle.max)}; }
    [[nodiscard]] inline ExactCircle exact(const Circle& circle) { return {exact(circle.center), circle.radius}; }

    [[nodiscard]] inline ExactRectangle moved(const ExactRectangle& rectangle, const Vector2d& offset) {
        return {rectangle.min + offset, rectangle.max + offset};
    }
    [[nodiscard]] inline ExactCircle moved(const ExactCircle& circle, const Vector2d& offset) {
        return {circle.center + offset, circle.radius};
    }

    // ------------------------------------------------------------------ signed distances

    /** @brief Distance from a point to a rectangle; inside, minus the distance to the nearest side. */
    [[nodiscard]] inline double signedDistance(const Vector2d& point, const ExactRectangle& rectangle) {
        // dx > 0: outside on X, by dx. dx <= 0: inside on X, -dx from the nearest vertical side.
        const double dx = std::max(rectangle.min.x - point.x, point.x - rectangle.max.x);
        const double dy = std::max(rectangle.min.y - point.y, point.y - rectangle.max.y);
        return std::hypot(std::max(dx, 0.0), std::max(dy, 0.0)) + std::min(std::max(dx, dy), 0.0);
    }

    /** @brief Distance from a point to a circle; inside, minus the distance to the edge. */
    [[nodiscard]] inline double signedDistance(const Vector2d& point, const ExactCircle& circle) {
        return (point - circle.center).length() - circle.radius;
    }

    /** @brief Distance between two rectangles: from b's center to a grown by b's half size. */
    [[nodiscard]] inline double signedDistance(const ExactRectangle& a, const ExactRectangle& b) {
        const Vector2d halfB = (b.max - b.min) * 0.5;
        return signedDistance((b.min + b.max) * 0.5, ExactRectangle{a.min - halfB, a.max + halfB});
    }

    /** @brief Distance between two circles: between the centers, minus both radii. */
    [[nodiscard]] inline double signedDistance(const ExactCircle& a, const ExactCircle& b) {
        return signedDistance(b.center, a) - b.radius;
    }

    /** @brief Distance between a rectangle and a circle: from the rectangle to the center, minus the radius. */
    [[nodiscard]] inline double signedDistance(const ExactRectangle& a, const ExactCircle& b) {
        return signedDistance(b.center, a) - b.radius;
    }

    [[nodiscard]] inline double signedDistance(const ExactCircle& a, const ExactRectangle& b) { return signedDistance(b, a); }

    /** @brief The point of the rectangle closest to point. */
    [[nodiscard]] inline Vector2d closestPoint(const ExactRectangle& rectangle, const Vector2d& point) {
        return {std::clamp(point.x, rectangle.min.x, rectangle.max.x), std::clamp(point.y, rectangle.min.y, rectangle.max.y)};
    }

    // ------------------------------------------------------------------ random cases

    /**
     * @brief Random shapes and motions, the same on every platform: splitmix64
     * rather than <random>, whose distributions differ between standard
     * libraries. A failing case found on Linux fails on Windows too.
     */
    class RandomCases {
    public:
        enum class Flavor {
            Continuous,  ///< any float
            Grid,        ///< multiples of 0.25: exact touches and ties, as with tiles
            AxisMotion,  ///< motions along X or Y only
            FarAway,     ///< around (1000, 1000), where a float step is 6e-5
        };

        RandomCases(Flavor flavor, std::uint64_t seed) : m_flavor(flavor), m_state(seed) {}

        /** @brief Below this distance, float rounding cannot decide: a few float steps at the coordinates' scale. */
        [[nodiscard]] double tolerance() const { return 1e-5 + 2e-6 * scale(); }

        /**
         * @brief How precise the normal of a curve of this radius can be: the
         * contact is only known to a few float steps, which turn the normal
         * by that much over the radius.
         */
        [[nodiscard]] double curveNormalTolerance(double radius) const {
            const double floatStep = std::ldexp(1.0, std::ilogb(scale()) - 23);
            return 1e-4 + 8.0 * floatStep / std::max(radius, 1e-9);
        }

        /** @brief A point within spread of this flavor's origin. */
        [[nodiscard]] Vector2f point(double spread) {
            return {snap(offset() + uniform(-spread, spread)), snap(offset() + uniform(-spread, spread))};
        }

        /** @brief A point within spread of around. */
        [[nodiscard]] Vector2f near(const Vector2f& around, double spread) {
            return {snap(around.x + uniform(-spread, spread)), snap(around.y + uniform(-spread, spread))};
        }

        [[nodiscard]] Rectangle rectangle(const Vector2f& center) { return Rectangle::fromCenter(center, {size(), size()}); }

        [[nodiscard]] Circle circle(const Vector2f& center) { return {center, size()}; }

        /** @brief A motion of up to 12 per axis; sometimes none. */
        [[nodiscard]] Vector2f motion() {
            if (chance(0.02)) {
                return {};
            }
            Vector2f delta{snap(uniform(-12.0, 12.0)), snap(uniform(-12.0, 12.0))};
            if (m_flavor == Flavor::AxisMotion) {
                (chance(0.5) ? delta.x : delta.y) = 0.f;
            }
            return delta;
        }

    private:
        Flavor m_flavor;
        std::uint64_t m_state;

        [[nodiscard]] double offset() const { return m_flavor == Flavor::FarAway ? 1000.0 : 0.0; }
        [[nodiscard]] double scale() const { return offset() + 20.0; }  // coordinates stay within ~20 of the offset

        /** @brief Uniform in [low, high): splitmix64, then 53 random bits as a double. */
        [[nodiscard]] double uniform(double low, double high) {
            std::uint64_t z = (m_state += 0x9E3779B97F4A7C15ULL);
            z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
            z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
            z ^= z >> 31;
            return low + (high - low) * static_cast<double>(z >> 11) * 0x1.0p-53;
        }

        [[nodiscard]] bool chance(double probability) { return uniform(0.0, 1.0) < probability; }

        [[nodiscard]] float snap(double value) const {
            return static_cast<float>(m_flavor == Flavor::Grid ? std::round(value * 4.0) / 4.0 : value);
        }

        /** @brief A half size or a radius, up to 3; sometimes 0 (a point, a segment). */
        [[nodiscard]] float size() {
            if (chance(0.03)) {
                return 0.f;
            }
            if (m_flavor == Flavor::Grid) {
                return static_cast<float>(std::max(0.25, std::round(uniform(0.0, 3.0) * 4.0) / 4.0));
            }
            return static_cast<float>(uniform(0.01, 3.0));
        }
    };

    /** @brief Runs test(random) on casesPerFlavor random cases of each flavor, with fixed seeds. */
    template <typename Test>
    void forRandomCases(int casesPerFlavor, const Test& test) {
        std::uint64_t seed = 20261110;
        for (const RandomCases::Flavor flavor :
             {RandomCases::Flavor::Continuous, RandomCases::Flavor::Grid, RandomCases::Flavor::AxisMotion,
              RandomCases::Flavor::FarAway}) {
            RandomCases random(flavor, seed++);
            for (int i = 0; i < casesPerFlavor; ++i) {
                test(random);
            }
        }
    }

    // ------------------------------------------------------------------ verdicts

    /** @brief What the oracle expects: clearly yes, clearly no, or too close to call. */
    enum class Expected { Yes, No, Either };

    /** @brief The oracle's expectation for one case, and what was wrong with the answer (empty if nothing). */
    struct Verdict {
        Expected expected = Expected::Either;
        std::string failure;
    };

    /** @brief Many random cases of one property: how many there were of each kind, how many failed, the first failure. */
    struct Tally {
        int cases = 0;
        int yes = 0;
        int no = 0;
        int failures = 0;
        std::string firstFailure;

        /** @brief Counts a case; describe() names it, called only for the first failure. */
        template <typename Describe>
        void record(const Verdict& verdict, const Describe& describe) {
            ++cases;
            yes += verdict.expected == Expected::Yes ? 1 : 0;
            no += verdict.expected == Expected::No ? 1 : 0;
            if (!verdict.failure.empty() && failures++ == 0) {
                firstFailure = verdict.failure + "\n    in " + describe();
            }
        }
    };

    [[nodiscard]] inline std::string withValue(const char* what, double value) {
        char buffer[160];
        std::snprintf(buffer, sizeof buffer, "%s (%.3g)", what, value);
        return buffer;
    }

    [[nodiscard]] inline Expected expectedFrom(double distance, double tolerance) {
        return distance < -tolerance ? Expected::Yes : distance > tolerance ? Expected::No : Expected::Either;
    }

    /** @brief Minimum of a function convex on [low, high], by ternary search. */
    template <typename F>
    [[nodiscard]] double minimumOf(const F& f, double low, double high) {
        for (int i = 0; i < 60; ++i) {
            const double a = low + (high - low) / 3.0;
            const double b = high - (high - low) / 3.0;
            if (f(a) < f(b)) {
                high = b;
            } else {
                low = a;
            }
        }
        return std::min({f(low), f(high), f((low + high) * 0.5)});
    }

    // ------------------------------------------------------------------ checks

    /** @brief A yes/no overlap answer, against the signed distance. */
    [[nodiscard]] inline Verdict checkOverlap(double distance, bool answer, double tolerance) {
        Verdict verdict{expectedFrom(distance, tolerance), {}};
        if (verdict.expected == Expected::Yes && !answer) {
            verdict.failure = withValue("no overlap, where they overlap by", -distance);
        } else if (verdict.expected == Expected::No && answer) {
            verdict.failure = withValue("an overlap, where they are apart by", distance);
        }
        return verdict;
    }

    /** @brief A contact between a and b (and overlaps(a, b)'s answer), against the signed distance. */
    template <typename ExactA, typename ExactB>
    [[nodiscard]] Verdict checkContact(
        const ExactA& a, const ExactB& b, const std::optional<Contact>& contact, bool overlapping, double tolerance) {
        const double distance = signedDistance(a, b);
        Verdict verdict{expectedFrom(distance, tolerance), {}};
        if (contact.has_value() != overlapping) {
            verdict.failure = withValue("collide and overlaps disagree, at distance", distance);
        } else if (verdict.expected == Expected::No && contact) {
            verdict.failure = withValue("a contact, where they are apart by", distance);
        } else if (verdict.expected == Expected::Yes && !contact) {
            verdict.failure = withValue("no contact, where they overlap by", -distance);
        } else if (contact) {
            const Vector2d normal = exact(contact->normal);
            const Vector2d point = exact(contact->point);
            const double shortestPush = std::max(-distance, 0.0);
            if (std::fabs(normal.length() - 1.0) > 1e-5) {
                verdict.failure = withValue("the normal is not unit", normal.length());
            } else if (std::fabs(contact->depth - shortestPush) > tolerance) {
                verdict.failure = withValue("the depth is not the shortest push that separates them, off by", contact->depth - shortestPush);
            } else if (const double after = signedDistance(a, moved(b, normal * double(contact->depth))); std::fabs(after) > tolerance) {
                verdict.failure = withValue("pushing b by normal * depth does not leave them touching, distance", after);
            } else if (std::max(signedDistance(point, a), signedDistance(point, b)) > tolerance) {
                verdict.failure = withValue("the point is outside the overlap, by", std::max(signedDistance(point, a), signedDistance(point, b)));
            }
        }
        return verdict;
    }

    /**
     * @brief The first contact of a ray or a sweep, given f(t): the signed
     * distance once the motion is done up to t, in [0, 1].
     */
    template <typename F>
    [[nodiscard]] Verdict checkFirstContact(const F& f, const std::optional<Hit>& hit, double tolerance) {
        const double atStart = f(0.0);
        const double closest = minimumOf(f, 0.0, 1.0);
        Verdict verdict;
        if (atStart < -tolerance || (atStart > tolerance && closest > tolerance)) {
            verdict.expected = Expected::No;  // starts inside (ignored), or never touches
        } else if (atStart > tolerance && closest < -tolerance) {
            verdict.expected = Expected::Yes;  // clearly goes in
        }  // else starts touching, or grazes: either answer

        if (verdict.expected == Expected::No && hit) {
            verdict.failure = withValue("a hit, where there is none, at fraction", hit->fraction);
        } else if (verdict.expected == Expected::Yes && !hit) {
            verdict.failure = withValue("no hit, where it goes in by", -closest);
        } else if (hit) {
            const double t = hit->fraction;
            if (!(t >= 0.0 && t <= 1.0)) {
                verdict.failure = withValue("the fraction is outside [0, 1]", t);
            } else if (std::fabs(f(t)) > tolerance) {
                verdict.failure = withValue("the shapes do not touch at the hit, distance", f(t));
            } else if (const double before = minimumOf(f, 0.0, t); before < -tolerance) {
                verdict.failure = withValue("the shapes overlap before the hit, by", -before);
            } else if (std::fabs(exact(hit->normal).length() - 1.0) > 1e-5) {
                verdict.failure = withValue("the normal is not unit", exact(hit->normal).length());
            }
        }
        return verdict;
    }

    /** @brief The point of a hit is where the motion is at its fraction: start + delta * fraction. */
    [[nodiscard]] inline std::string checkHitPoint(const Hit& hit, const Vector2d& start, const Vector2d& delta, double tolerance) {
        const double off = (exact(hit.point) - (start + delta * double(hit.fraction))).length();
        return off > tolerance ? withValue("the point is not start + delta * fraction, off by", off) : std::string();
    }

    /** @brief A hit on a rectangle's side: the normal is the outward normal of a side the contact is on, against the motion. */
    [[nodiscard]] inline std::string checkSideNormal(
        const Vector2f& normal, const Vector2d& contact, const ExactRectangle& rectangle, const Vector2d& delta, double tolerance) {
        const auto on = [tolerance](double a, double b) { return std::fabs(a - b) <= tolerance; };
        const bool isSide = (normal == Vector2f(-1.f, 0.f) && on(contact.x, rectangle.min.x)) ||
                            (normal == Vector2f(1.f, 0.f) && on(contact.x, rectangle.max.x)) ||
                            (normal == Vector2f(0.f, -1.f) && on(contact.y, rectangle.min.y)) ||
                            (normal == Vector2f(0.f, 1.f) && on(contact.y, rectangle.max.y));
        if (!isSide) {
            return "the normal is not the outward normal of a side the contact is on";
        }
        return exact(normal).dot(delta) > 0.0 ? "the normal faces the same way as the motion" : std::string();
    }

    /** @brief A hit on a curve: the normal points along outward, within tolerance. */
    [[nodiscard]] inline std::string checkCurveNormal(const Vector2f& normal, const Vector2d& outward, double tolerance) {
        const double length = outward.length();
        if (length < 1e-9) {
            return {};  // no direction to compare with
        }
        const double off = (exact(normal) - outward / length).length();
        return off > tolerance ? withValue("the normal is off the curve's normal by", off) : std::string();
    }

    /** @brief The verdict, with another check's failure if it had none yet. */
    [[nodiscard]] inline Verdict alsoCheck(Verdict verdict, const std::string& otherFailure) {
        if (verdict.failure.empty()) {
            verdict.failure = otherFailure;
        }
        return verdict;
    }

    // ------------------------------------------------------------------ descriptions, pasteable into a test

    [[nodiscard]] inline std::string describe(const Vector2f& v) {
        char buffer[96];
        std::snprintf(buffer, sizeof buffer, "{%.9gf, %.9gf}", double(v.x), double(v.y));
        return buffer;
    }

    [[nodiscard]] inline std::string describe(const Rectangle& r) {
        return "Rectangle{" + describe(r.min) + ", " + describe(r.max) + "}";
    }

    [[nodiscard]] inline std::string describe(const Circle& c) {
        char buffer[48];
        std::snprintf(buffer, sizeof buffer, ", %.9gf}", double(c.radius));
        return "Circle{" + describe(c.center) + buffer;
    }

    /** @brief "name(a, b, c)", each argument described. */
    template <typename... Arguments>
    [[nodiscard]] std::string describeCall(const char* name, const Arguments&... arguments) {
        std::string text = std::string(name) + "(";
        bool first = true;
        ((text += (first ? "" : ", ") + describe(arguments), first = false), ...);
        return text + ")";
    }
}
