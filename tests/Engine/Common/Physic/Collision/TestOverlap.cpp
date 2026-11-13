#include <boost/ut.hpp>

#include "Engine/Common/Physic/Collision/Overlap.hpp"
#include "../../../Support/CollisionOracle.hpp"

using namespace boost::ut;
using namespace Engine::Collision;
using namespace TestSupport;

namespace {
constexpr int RandomCasesPerFlavor = 5000;

void test_contains() {
    "contains includes the edges of a rectangle"_test = [] {
        constexpr Rectangle rectangle{{0.f, 0.f}, {2.f, 1.f}};

        expect(constant<contains(rectangle, Vector2f(1.f, 0.5f))>);
        expect(constant<contains(rectangle, Vector2f(0.f, 0.f))>);
        expect(constant<contains(rectangle, Vector2f(2.f, 0.5f))>);
        expect(constant<!contains(rectangle, Vector2f(2.001f, 0.5f))>);
    };

    "contains includes the edge of a circle"_test = [] {
        constexpr Circle circle{{0.f, 0.f}, 1.f};

        expect(constant<contains(circle, Vector2f(0.5f, 0.5f))>);
        expect(constant<contains(circle, Vector2f(0.f, 1.f))>);
        expect(constant<!contains(circle, Vector2f(0.8f, 0.8f))>);
    };
}

void test_overlaps() {
    "rectangles overlap when they share an area or only an edge"_test = [] {
        constexpr Rectangle a{{0.f, 0.f}, {1.f, 1.f}};

        expect(constant<overlaps(a, Rectangle{{0.5f, 0.5f}, {2.f, 2.f}})>);
        expect(constant<overlaps(a, Rectangle{{1.f, 0.f}, {2.f, 1.f}})>);
        expect(constant<overlaps(a, Rectangle{{1.f, 1.f}, {2.f, 2.f}})>);
        expect(constant<!overlaps(a, Rectangle{{1.1f, 0.f}, {2.f, 1.f}})>);
        expect(constant<!overlaps(a, Rectangle{{0.f, -2.f}, {1.f, -0.1f}})>);
    };

    "circles overlap when their centers are at most the sum of the radii apart"_test = [] {
        constexpr Circle a{{0.f, 0.f}, 1.f};

        expect(constant<overlaps(a, Circle{{1.5f, 0.f}, 1.f})>);
        expect(constant<overlaps(a, Circle{{3.f, 0.f}, 2.f})>);
        expect(constant<!overlaps(a, Circle{{3.f, 0.f}, 1.f})>);
    };

    "a rectangle and a circle overlap through a side or a corner"_test = [] {
        constexpr Rectangle rectangle{{0.f, 0.f}, {2.f, 2.f}};

        expect(constant<overlaps(rectangle, Circle{{2.5f, 1.f}, 1.f})>);
        expect(constant<overlaps(rectangle, Circle{{2.5f, 2.5f}, 1.f})>);
        expect(constant<overlaps(rectangle, Circle{{1.f, 1.f}, 0.1f})>);
        expect(constant<overlaps(Circle{{2.5f, 1.f}, 1.f}, rectangle)>);
    };

    "a circle near a corner does not overlap the rectangle even when its bounding box does"_test = [] {
        constexpr Rectangle rectangle{{0.f, 0.f}, {1.f, 1.f}};
        constexpr Circle circle{{1.8f, 1.8f}, 1.f};

        expect(constant<overlaps(rectangle, circle.boundingBox())>);
        expect(constant<!overlaps(rectangle, circle)>);
        expect(constant<!overlaps(circle, rectangle)>);
    };
}

void test_random_cases() {
    "on random shapes, overlaps and contains agree with the signed distance"_test = [] {
        Tally rectangles, circles, mixed, inRectangle, inCircle;
        forRandomCases(RandomCasesPerFlavor, [&](RandomCases& random) {
            const double tolerance = random.tolerance();
            const Vector2f home = random.point(6.0);
            const Rectangle a = random.rectangle(home);
            const Rectangle b = random.rectangle(random.near(home, 5.0));
            const Circle c = random.circle(home);
            const Circle d = random.circle(random.near(home, 5.0));
            const Vector2f point = random.near(home, 4.0);

            rectangles.record(checkOverlap(signedDistance(exact(a), exact(b)), overlaps(a, b), tolerance),
                              [&] { return describeCall("overlaps", a, b); });
            circles.record(checkOverlap(signedDistance(exact(c), exact(d)), overlaps(c, d), tolerance),
                           [&] { return describeCall("overlaps", c, d); });
            mixed.record(checkOverlap(signedDistance(exact(a), exact(d)), overlaps(a, d), tolerance),
                         [&] { return describeCall("overlaps", a, d); });
            mixed.record(checkOverlap(signedDistance(exact(c), exact(b)), overlaps(c, b), tolerance),
                         [&] { return describeCall("overlaps", c, b); });
            inRectangle.record(checkOverlap(signedDistance(exact(point), exact(a)), contains(a, point), tolerance),
                               [&] { return describeCall("contains", a, point); });
            inCircle.record(checkOverlap(signedDistance(exact(point), exact(c)), contains(c, point), tolerance),
                            [&] { return describeCall("contains", c, point); });
        });
        for (const Tally* tally : {&rectangles, &circles, &mixed, &inRectangle, &inCircle}) {
            expect(tally->failures == 0_i) << tally->firstFailure;
            expect(tally->yes > 0 && tally->no > 0) << "the random cases must give both answers";
        }
    };
}
}

void run_collision_overlap_tests() {
    test_contains();
    test_overlaps();
    test_random_cases();
}
