#include <boost/ut.hpp>

#include "Engine/Common/Physic/Collision/Raycast.hpp"
#include "../../../Support/Approx.hpp"
#include "../../../Support/CollisionOracle.hpp"

using namespace boost::ut;
using namespace Engine::Collision;
using namespace TestSupport;

namespace {
constexpr float Tolerance = 1e-5f;
constexpr int RandomCasesPerFlavor = 5000;
constexpr Rectangle Box{{2.f, -1.f}, {4.f, 1.f}};
constexpr Circle Disc{{5.f, 0.f}, 1.f};

void test_ray_box_hits() {
    "a ray hits the face it reaches first"_test = [] {
        const std::optional<Hit> hit = raycast({0.f, 0.f}, {10.f, 0.f}, Box);

        expect(hit.has_value() >> fatal);
        expect(near(hit->fraction, 0.2f, Tolerance));
        expect(near(hit->point, Vector2f(2.f, 0.f), Tolerance));
        expect(hit->normal == Vector2f(-1.f, 0.f));
    };

    "a vertical ray hits the top face"_test = [] {
        const std::optional<Hit> hit = raycast({3.f, 5.f}, {0.f, -10.f}, Box);

        expect(hit.has_value() >> fatal);
        expect(near(hit->fraction, 0.4f, Tolerance));
        expect(near(hit->point, Vector2f(3.f, 1.f), Tolerance));
        expect(hit->normal == Vector2f(0.f, 1.f));
    };

    "a ray hitting a corner exactly reports the horizontal face (ties go to Y)"_test = [] {
        const std::optional<Hit> hit = raycast({0.f, 3.f}, {4.f, -4.f}, Box);

        expect(hit.has_value() >> fatal);
        expect(near(hit->fraction, 0.5f, Tolerance));
        expect(hit->normal == Vector2f(0.f, 1.f));
    };

    "a ray starting on the surface and going in hits at fraction 0"_test = [] {
        const std::optional<Hit> hit = raycast({2.f, 0.f}, {1.f, 0.f}, Box);

        expect(hit.has_value() >> fatal);
        expect(hit->fraction == 0.f);
        expect(hit->normal == Vector2f(-1.f, 0.f));
    };
}

void test_ray_box_misses() {
    "a ray passing beside, too short or going away misses"_test = [] {
        expect(!raycast({0.f, 2.f}, {10.f, 0.f}, Box).has_value());
        expect(!raycast({0.f, 0.f}, {1.f, 0.f}, Box).has_value());
        expect(!raycast({0.f, 0.f}, {-10.f, 0.f}, Box).has_value());
        expect(!raycast({0.f, 0.f}, {0.f, 0.f}, Box).has_value());
    };

    "a ray parallel to the sides passes on either side of the box"_test = [] {
        expect(!raycast({0.f, 0.f}, {0.f, 5.f}, Box).has_value());    // vertical, left of the box
        expect(!raycast({5.f, -3.f}, {0.f, 5.f}, Box).has_value());   // vertical, right of the box
        expect(!raycast({0.f, -2.f}, {10.f, 0.f}, Box).has_value());  // horizontal, below the box
    };

    "a ray starting inside the box, or leaving it from its surface, ignores it"_test = [] {
        expect(!raycast({3.f, 0.f}, {10.f, 0.f}, Box).has_value());
        expect(!raycast({4.f, 0.f}, {1.f, 0.f}, Box).has_value());
    };
}

void test_ray_circle() {

    "a ray hits a circle where it first reaches its edge"_test = [] {
        const std::optional<Hit> hit = raycast({0.f, 0.f}, {10.f, 0.f}, Disc);

        expect(hit.has_value() >> fatal);
        expect(near(hit->fraction, 0.4f, Tolerance));
        expect(near(hit->point, Vector2f(4.f, 0.f), Tolerance));
        expect(near(hit->normal, Vector2f(-1.f, 0.f), Tolerance));
    };

    "a ray grazing the circle hits it at the tangent point"_test = [] {
        const std::optional<Hit> hit = raycast({0.f, 1.f}, {10.f, 0.f}, Disc);

        expect(hit.has_value() >> fatal);
        expect(near(hit->point, Vector2f(5.f, 1.f), Tolerance));
        expect(near(hit->normal, Vector2f(0.f, 1.f), Tolerance));
    };

    "a ray passing beside, going away, too short or starting inside misses"_test = [] {
        expect(!raycast({0.f, 2.f}, {10.f, 0.f}, Disc).has_value());
        expect(!raycast({0.f, 0.f}, {-10.f, 0.f}, Disc).has_value());
        expect(!raycast({0.f, 0.f}, {3.f, 0.f}, Disc).has_value());
        expect(!raycast({5.f, 0.5f}, {10.f, 0.f}, Disc).has_value());
    };

    "a small circle far away is hit precisely"_test = [] {
        // 3-4-5 triangle: the ray y = 0.06 meets the circle of radius 0.1 at x = -0.08.
        // The textbook b² - a c lost 2e-5 here, in two huge, nearly equal numbers.
        const std::optional<Hit> hit = raycast({-7.f, 0.06f}, {10.f, 0.f}, Circle{{0.f, 0.f}, 0.1f});

        expect(hit.has_value() >> fatal);
        expect(near(hit->point, Vector2f(-0.08f, 0.06f), 1e-6f));
    };

    "a tiny circle, and a point, still give a unit normal"_test = [] {
        const std::optional<Hit> tiny = raycast({-1.f, 0.f}, {2.f, 0.f}, Circle{{0.f, 0.f}, 1e-6f});
        expect(tiny.has_value() >> fatal);
        expect(near(tiny->normal, Vector2f(-1.f, 0.f), Tolerance));

        // A point has no surface: the normal faces the ray.
        const std::optional<Hit> point = raycast({0.f, -1.f}, {0.f, 4.f}, Circle{{0.f, 1.5f}, 0.f});
        expect(point.has_value() >> fatal);
        expect(near(point->fraction, 0.625f, Tolerance));
        expect(near(point->normal, Vector2f(0.f, -1.f), Tolerance));
    };
}

void test_random_cases() {
    "on random rays, the hit is the first contact, with the surface's normal"_test = [] {
        Tally rectangles, circles;
        forRandomCases(RandomCasesPerFlavor, [&](RandomCases& random) {
            const double tolerance = random.tolerance();
            const Vector2f home = random.point(6.0);
            const Rectangle rectangle = random.rectangle(home);
            const Circle circle = random.circle(home);
            const Vector2f origin = random.near(home, 8.0);
            const Vector2f delta = random.motion();
            const Vector2d start = exact(origin);
            const Vector2d motion = exact(delta);

            const ExactRectangle exactRectangle = exact(rectangle);
            const std::optional<Hit> onRectangle = raycast(origin, delta, rectangle);
            Verdict verdict = checkFirstContact(
                [&](double t) { return signedDistance(start + motion * t, exactRectangle); }, onRectangle, tolerance);
            if (onRectangle) {
                verdict = alsoCheck(verdict, checkHitPoint(*onRectangle, start, motion, tolerance));
                if (verdict.expected == Expected::Yes) {
                    verdict = alsoCheck(verdict, checkSideNormal(onRectangle->normal, exact(onRectangle->point), exactRectangle, motion, tolerance));
                }
            }
            rectangles.record(verdict, [&] { return describeCall("raycast", origin, delta, rectangle); });

            const ExactCircle exactCircle = exact(circle);
            const std::optional<Hit> onCircle = raycast(origin, delta, circle);
            verdict = checkFirstContact(
                [&](double t) { return signedDistance(start + motion * t, exactCircle); }, onCircle, tolerance);
            if (onCircle) {
                verdict = alsoCheck(verdict, checkHitPoint(*onCircle, start, motion, tolerance));
                if (verdict.expected == Expected::Yes) {
                    verdict = alsoCheck(verdict, checkCurveNormal(onCircle->normal, exact(onCircle->point) - exactCircle.center,
                                                                  random.curveNormalTolerance(circle.radius)));
                }
            }
            circles.record(verdict, [&] { return describeCall("raycast", origin, delta, circle); });
        });
        for (const Tally* tally : {&rectangles, &circles}) {
            expect(tally->failures == 0_i) << tally->firstFailure;
            expect(tally->yes > 0 && tally->no > 0) << "the random cases must give both answers";
        }
    };
}
}

void run_collision_raycast_tests() {
    test_ray_box_hits();
    test_ray_box_misses();
    test_ray_circle();
    test_random_cases();
}
