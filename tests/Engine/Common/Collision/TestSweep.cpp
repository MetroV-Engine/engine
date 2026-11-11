#include <boost/ut.hpp>

#include <cmath>

#include "Engine/Common/Collision/Collide.hpp"
#include "Engine/Common/Collision/Overlap.hpp"
#include "Engine/Common/Collision/Sweep.hpp"
#include "../../Support/Approx.hpp"
#include "../../Support/CollisionOracle.hpp"

using namespace boost::ut;
using namespace Engine::Collision;
using namespace TestSupport;

namespace {
constexpr float Tolerance = 1e-5f;
constexpr int RandomCasesPerFlavor = 5000;

void test_box_sweeps() {
    "a falling box stops exactly on the floor"_test = [] {
        const Rectangle floor{{-10.f, -1.f}, {10.f, 0.f}};
        const Rectangle box = Rectangle::fromCenter({0.f, 3.f}, {0.5f, 0.5f});

        const std::optional<Hit> hit = sweep(box, {0.f, -5.f}, floor);

        expect(hit.has_value() >> fatal);
        expect(near(hit->fraction, 0.5f, Tolerance));
        expect(near(hit->point, Vector2f(0.f, 0.5f), Tolerance));
        expect(hit->normal == Vector2f(0.f, 1.f));

        const std::optional<Contact> atImpact = collide(box.translated({0.f, -5.f * hit->fraction}), floor);
        expect(atImpact.has_value() >> fatal);
        expect(near(atImpact->depth, 0.f, Tolerance));
    };

    "a fast box does not tunnel through a thin wall"_test = [] {
        const Rectangle wall{{5.f, -2.f}, {5.1f, 2.f}};
        const Rectangle box = Rectangle::fromCenter({0.f, 0.f}, {0.5f, 0.5f});
        const Vector2f delta{20.f, 0.f};

        // Tested only at the start and at the end, the box never touches the wall.
        expect(!overlaps(box, wall));
        expect(!overlaps(box.translated(delta), wall));

        const std::optional<Hit> hit = sweep(box, delta, wall);
        expect(hit.has_value() >> fatal);
        expect(near(hit->fraction, 0.225f, Tolerance));
        expect(hit->normal == Vector2f(-1.f, 0.f));
    };

    "boxes already overlapping, or never meeting, give no hit"_test = [] {
        const Rectangle wall{{5.f, -2.f}, {6.f, 2.f}};

        expect(!sweep(Rectangle::fromCenter({5.5f, 0.f}, {0.5f, 0.5f}), {10.f, 0.f}, wall).has_value());
        expect(!sweep(Rectangle::fromCenter({0.f, 5.f}, {0.5f, 0.5f}), {10.f, 0.f}, wall).has_value());
        expect(!sweep(Rectangle::fromCenter({0.f, 0.f}, {0.5f, 0.5f}), {3.f, 0.f}, wall).has_value());
    };
}

void test_circle_sweeps() {
    "a moving circle stops when it touches a still circle"_test = [] {
        const std::optional<Hit> hit = sweep(Circle{{0.f, 0.f}, 1.f}, {10.f, 0.f}, Circle{{6.f, 0.f}, 1.f});

        expect(hit.has_value() >> fatal);
        expect(near(hit->fraction, 0.4f, Tolerance));
        expect(near(hit->point, Vector2f(4.f, 0.f), Tolerance));
        expect(near(hit->normal, Vector2f(-1.f, 0.f), Tolerance));
    };

    "a circle stops on the face of a box"_test = [] {
        const std::optional<Hit> hit = sweep(Circle{{0.f, 0.f}, 0.5f}, {10.f, 0.f}, Rectangle{{3.f, -1.f}, {5.f, 1.f}});

        expect(hit.has_value() >> fatal);
        expect(near(hit->fraction, 0.25f, Tolerance));
        expect(hit->normal == Vector2f(-1.f, 0.f));
    };

    "a circle reaching a corner stops on the rounded corner, not on the grown box"_test = [] {
        const Rectangle box{{0.f, 0.f}, {2.f, 2.f}};

        const std::optional<Hit> hit = sweep(Circle{{4.f, 4.f}, 1.f}, {-4.f, -4.f}, box);

        // Contact when the center is 1 from the corner (2, 2), along the diagonal.
        const float diagonal = std::sqrt(0.5f);
        expect(hit.has_value() >> fatal);
        expect(near(hit->fraction, (2.f - diagonal) / 4.f, Tolerance));
        expect(near(hit->normal, Vector2f(diagonal, diagonal), Tolerance));
        expect(near(hit->point, Vector2f(2.f + diagonal, 2.f + diagonal), Tolerance));
    };

    "a circle passing near a corner misses it, though it crosses the grown box"_test = [] {
        const Rectangle box{{0.f, 0.f}, {2.f, 2.f}};
        const Circle circle{{1.95f, 3.9f}, 1.f};
        const Vector2f delta{2.f, -2.f};

        expect(raycast(circle.center, delta, box.expanded({1.f, 1.f})).has_value());
        expect(!sweep(circle, delta, box).has_value());
    };

    "circles and boxes already overlapping, or never meeting, give no hit"_test = [] {
        const Rectangle box{{0.f, 0.f}, {2.f, 2.f}};
        const Circle disc{{6.f, 1.f}, 1.f};

        expect(!sweep(Circle{{2.2f, 1.f}, 0.5f}, {5.f, 0.f}, box).has_value());
        expect(!sweep(Circle{{0.f, 5.f}, 0.5f}, {5.f, 0.f}, box).has_value());
        expect(!sweep(Rectangle::fromCenter({5.5f, 1.f}, {1.f, 1.f}), {5.f, 0.f}, disc).has_value());
        expect(!sweep(Rectangle::fromCenter({0.f, 5.f}, {0.5f, 0.5f}), {10.f, 0.f}, disc).has_value());
    };

    "a circle reaching the side exactly level with the top gets the side's normal"_test = [] {
        // At t = 0.5 the center is at (3, 0): level with the top, touching the corner (2, 0)
        // from the side. The wide piece alone would say "top" (its corner tie goes to Y).
        const std::optional<Hit> hit = sweep(Circle{{5.f, 1.f}, 1.f}, {-4.f, -2.f}, Rectangle{{0.f, -2.f}, {2.f, 0.f}});

        expect(hit.has_value() >> fatal);
        expect(near(hit->fraction, 0.5f, Tolerance));
        expect(hit->normal == Vector2f(1.f, 0.f));
    };

    "far from the origin, a circle hitting a side next to a corner gets the side's exact normal"_test = [] {
        // About 1000 m out, a float step is 6e-5 m: next to the corner, the side and the
        // corner circle are closer than that, and the corner circle may find the contact.
        const std::optional<Hit> hit = sweep(Circle{{997.647522f, 991.397827f}, 1.08307588f}, {-8.44289207f, 11.6432877f},
                                             Rectangle{{994.113037f, 997.351685f}, {997.453247f, 1000.38904f}});

        expect(hit.has_value() >> fatal);
        expect(hit->normal == Vector2f(0.f, -1.f));
    };

    "a circle of radius 0 sweeps like a ray"_test = [] {
        const Rectangle box{{0.f, 0.f}, {2.f, 2.f}};

        // Starts inside and leaves exactly through the corner (2, 2): ignored, as a ray.
        expect(!sweep(Circle{{1.f, 1.f}, 0.f}, {2.f, 2.f}, box).has_value());

        const std::optional<Hit> hit = sweep(Circle{{-1.f, 1.f}, 0.f}, {4.f, 0.f}, box);
        expect(hit.has_value() >> fatal);
        expect(near(hit->fraction, 0.25f, Tolerance));
        expect(hit->normal == Vector2f(-1.f, 0.f));
    };

    "a box sweeping a circle gets the circle's normal, pointing back at the box"_test = [] {
        const Rectangle box = Rectangle::fromCenter({0.f, 0.f}, {0.5f, 0.5f});

        const std::optional<Hit> hit = sweep(box, {10.f, 0.f}, Circle{{5.f, 0.f}, 1.f});

        expect(hit.has_value() >> fatal);
        expect(near(hit->fraction, 0.35f, Tolerance));
        expect(near(hit->point, Vector2f(3.5f, 0.f), Tolerance));
        expect(near(hit->normal, Vector2f(-1.f, 0.f), Tolerance));
    };

    // At the time of impact the shapes touch: a little before, they are apart;
    // a little after, they overlap. (Exactly at it, float rounding may leave a
    // 1e-7 gap or overlap either way.)
    "the time of impact is when the shapes start touching"_test = [] {
        const Rectangle box{{0.f, 0.f}, {2.f, 2.f}};
        for (const Vector2f& delta : {Vector2f(-2.f, -4.f), Vector2f(-3.f, -6.f), Vector2f(-6.f, -2.f)}) {
            const Circle moving{{4.f, 4.f}, 0.75f};
            const std::optional<Hit> hit = sweep(moving, delta, box);
            expect(hit.has_value() >> fatal);
            const Vector2f step = delta.normalized() * Tolerance;
            expect(!overlaps(box, Circle{hit->point - step, moving.radius}));
            expect(overlaps(box, Circle{hit->point + step, moving.radius}));
        }

        const Circle still{{6.f, 1.f}, 1.f};
        const Rectangle movingBox = Rectangle::fromCenter({1.f, 4.f}, {0.5f, 0.5f});
        const Vector2f delta{6.f, -3.f};
        const std::optional<Hit> hit = sweep(movingBox, delta, still);
        expect(hit.has_value() >> fatal);
        const Vector2f step = delta.normalized() * Tolerance;
        expect(!overlaps(Rectangle::fromCenter(hit->point - step, movingBox.halfSize()), still));
        expect(overlaps(Rectangle::fromCenter(hit->point + step, movingBox.halfSize()), still));
    };
}

void test_random_cases() {
    "on random sweeps, the hit is the first contact, with the surface's normal"_test = [] {
        Tally rectangles, circles, circleRectangle, rectangleCircle;
        forRandomCases(RandomCasesPerFlavor, [&](RandomCases& random) {
            const double tolerance = random.tolerance();
            const Vector2f home = random.point(6.0);
            const Rectangle movingRectangle = random.rectangle(random.near(home, 8.0));
            const Circle movingCircle = random.circle(random.near(home, 8.0));
            const Rectangle rectangle = random.rectangle(home);
            const Circle circle = random.circle(home);
            const Vector2f delta = random.motion();
            const Vector2d motion = exact(delta);
            const ExactRectangle exactMovingRectangle = exact(movingRectangle);
            const ExactCircle exactMovingCircle = exact(movingCircle);
            const ExactRectangle exactRectangle = exact(rectangle);
            const ExactCircle exactCircle = exact(circle);
            const Vector2d rectangleStart = (exactMovingRectangle.min + exactMovingRectangle.max) * 0.5;

            // Rectangle into rectangle: the center hits a side of the target grown by the moving half size.
            const std::optional<Hit> h1 = sweep(movingRectangle, delta, rectangle);
            Verdict verdict = checkFirstContact(
                [&](double t) { return signedDistance(exactRectangle, moved(exactMovingRectangle, motion * t)); }, h1, tolerance);
            if (h1) {
                verdict = alsoCheck(verdict, checkHitPoint(*h1, rectangleStart, motion, tolerance));
                if (verdict.expected == Expected::Yes) {
                    const Vector2d half = (exactMovingRectangle.max - exactMovingRectangle.min) * 0.5;
                    const ExactRectangle grown{exactRectangle.min - half, exactRectangle.max + half};
                    verdict = alsoCheck(verdict, checkSideNormal(h1->normal, exact(h1->point), grown, motion, tolerance));
                }
            }
            rectangles.record(verdict, [&] { return describeCall("sweep", movingRectangle, delta, rectangle); });

            // Circle into circle: the normal goes from the target's center to the moving center.
            const std::optional<Hit> h2 = sweep(movingCircle, delta, circle);
            verdict = checkFirstContact(
                [&](double t) { return signedDistance(exactCircle, moved(exactMovingCircle, motion * t)); }, h2, tolerance);
            if (h2) {
                verdict = alsoCheck(verdict, checkHitPoint(*h2, exactMovingCircle.center, motion, tolerance));
                if (verdict.expected == Expected::Yes) {
                    verdict = alsoCheck(verdict, checkCurveNormal(h2->normal, exact(h2->point) - exactCircle.center,
                                                                  random.curveNormalTolerance(double(movingCircle.radius) + circle.radius)));
                }
            }
            circles.record(verdict, [&] { return describeCall("sweep", movingCircle, delta, circle); });

            // Circle into rectangle: the normal goes from the rectangle's closest point to the moving center.
            const std::optional<Hit> h3 = sweep(movingCircle, delta, rectangle);
            verdict = checkFirstContact(
                [&](double t) { return signedDistance(exactRectangle, moved(exactMovingCircle, motion * t)); }, h3, tolerance);
            if (h3) {
                verdict = alsoCheck(verdict, checkHitPoint(*h3, exactMovingCircle.center, motion, tolerance));
                if (verdict.expected == Expected::Yes) {
                    const Vector2d center = exact(h3->point);
                    verdict = alsoCheck(verdict, checkCurveNormal(h3->normal, center - closestPoint(exactRectangle, center),
                                                                  random.curveNormalTolerance(movingCircle.radius)));
                }
            }
            circleRectangle.record(verdict, [&] { return describeCall("sweep", movingCircle, delta, rectangle); });

            // Rectangle into circle: the normal goes from the circle's center to the moving rectangle's closest point.
            const std::optional<Hit> h4 = sweep(movingRectangle, delta, circle);
            verdict = checkFirstContact(
                [&](double t) { return signedDistance(moved(exactMovingRectangle, motion * t), exactCircle); }, h4, tolerance);
            if (h4) {
                verdict = alsoCheck(verdict, checkHitPoint(*h4, rectangleStart, motion, tolerance));
                if (verdict.expected == Expected::Yes) {
                    const ExactRectangle atImpact = moved(exactMovingRectangle, motion * double(h4->fraction));
                    verdict = alsoCheck(verdict, checkCurveNormal(h4->normal, closestPoint(atImpact, exactCircle.center) - exactCircle.center,
                                                                  random.curveNormalTolerance(circle.radius)));
                }
            }
            rectangleCircle.record(verdict, [&] { return describeCall("sweep", movingRectangle, delta, circle); });
        });
        for (const Tally* tally : {&rectangles, &circles, &circleRectangle, &rectangleCircle}) {
            expect(tally->failures == 0_i) << tally->firstFailure;
            expect(tally->yes > 0 && tally->no > 0) << "the random cases must give both answers";
        }
    };
}
}

void run_collision_sweep_tests() {
    test_box_sweeps();
    test_circle_sweeps();
    test_random_cases();
}
