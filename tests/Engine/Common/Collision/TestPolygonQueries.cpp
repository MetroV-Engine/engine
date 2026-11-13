#include <boost/ut.hpp>

#include <cmath>
#include <limits>
#include <optional>

#include "Engine/Common/Collision/Collision.hpp"
#include "../../Support/Approx.hpp"
#include "../../Support/CollisionOracle.hpp"

using namespace boost::ut;
using namespace Engine::Collision;
using namespace TestSupport;

namespace {
constexpr float Tolerance = 1e-5f;
constexpr int RandomCasesPerFlavor = 3000;
constexpr int RandomSweepsPerFlavor = 1000;
const float Diagonal = std::sqrt(0.5f);

// A slope rising to the right: its surface is the line y = x, from (0, 0) to (2, 2).
constexpr Polygon Slope{{0.f, 0.f}, {2.f, 0.f}, {2.f, 2.f}};

void test_overlap() {
    "contains and overlaps follow the slope, edges included"_test = [] {
        expect(constant<contains(Slope, Vector2f(1.5f, 0.5f))>);
        expect(constant<contains(Slope, Vector2f(1.f, 1.f))>);
        expect(constant<!contains(Slope, Vector2f(0.5f, 1.5f))>);

        // A box above the slope: its bottom-right corner (1, 1) is on it.
        expect(constant<overlaps(Slope, Rectangle{{0.f, 1.f}, {1.f, 2.f}})>);
        expect(constant<!overlaps(Slope, Rectangle{{0.f, 1.01f}, {0.99f, 2.f}})>);
        expect(constant<overlaps(Rectangle{{1.f, -1.f}, {3.f, 0.f}}, Slope)>) << "touching the bottom";

        // A circle above the slope, its center at distance 1 from it.
        expect(overlaps(Slope, Circle{{1.f - Diagonal, 1.f + Diagonal}, 1.01f}));
        expect(!overlaps(Circle{{1.f - Diagonal, 1.f + Diagonal}, 0.99f}, Slope));

        expect(constant<overlaps(Slope, Polygon{{1.f, 1.f}, {1.f, 0.5f}, {3.f, 3.f}})>) << "a vertex inside";
        expect(constant<!overlaps(Slope, Polygon{{0.f, 0.5f}, {1.f, 1.6f}, {0.f, 2.f}})>);
    };
}

void test_collide() {
    "a box sunk into a slope is pushed out along the slope's normal"_test = [] {
        // Its bottom-right corner (1.75, 1.7) is 0.05 below the line y = x; the rest is above.
        const Rectangle box{{1.25f, 1.7f}, {1.75f, 2.2f}};
        const std::optional<Contact> contact = collide(Slope, box);

        expect(contact.has_value() >> fatal);
        expect(near(contact->normal, Vector2f(-Diagonal, Diagonal), Tolerance));
        expect(near(contact->depth, 0.05f * Diagonal, Tolerance));
        expect(contains(Slope, contact->point) || near(contact->point.y, contact->point.x, Tolerance));

        const std::optional<Contact> reversed = collide(box, Slope);
        expect(reversed.has_value() >> fatal);
        expect(near(reversed->normal, Vector2f(Diagonal, -Diagonal), Tolerance));
        expect(near(reversed->depth, contact->depth, Tolerance));
    };

    "a rectangle given as a polygon gets exactly the contact of collide(Rectangle, Rectangle)"_test = [] {
        int checked = 0;
        for (float x = -2.f; x <= 2.f; x += 0.25f) {
            for (float y = -2.f; y <= 2.f; y += 0.25f) {
                const Rectangle a{{-1.f, -1.f}, {1.f, 0.5f}};
                const Rectangle b = Rectangle::fromCenter({x, y}, {0.75f, 0.5f});
                const std::optional<Contact> boxes = collide(a, b);
                const std::optional<Contact> polygons = collide(Polygon::fromRectangle(a), Polygon::fromRectangle(b));
                expect(boxes.has_value() == polygons.has_value()) << x << y;
                if (boxes && polygons) {
                    expect(polygons->normal == boxes->normal) << "at" << x << y;
                    expect(polygons->depth == boxes->depth) << "at" << x << y;
                    expect(near(polygons->point, boxes->point, Tolerance)) << "at" << x << y;
                    ++checked;
                }
            }
        }
        expect(checked > 100_i);
    };

    "a circle on a slope is pushed along its normal; on a vertex, away from the vertex"_test = [] {
        // Center 0.5 from the slope, radius 0.6: 0.1 deep.
        const Circle onFace{{1.f - 0.5f * Diagonal, 1.f + 0.5f * Diagonal}, 0.6f};
        const std::optional<Contact> face = collide(Slope, onFace);
        expect(face.has_value() >> fatal);
        expect(near(face->normal, Vector2f(-Diagonal, Diagonal), Tolerance));
        expect(near(face->depth, 0.1f, Tolerance));

        const Circle onTop{{2.3f, 2.4f}, 0.6f};
        const std::optional<Contact> vertex = collide(Slope, onTop);
        expect(vertex.has_value() >> fatal);
        expect(near(vertex->normal, Vector2f(0.6f, 0.8f), Tolerance));
        expect(near(vertex->depth, 0.1f, Tolerance));

        const std::optional<Contact> reversed = collide(onTop, Slope);
        expect(reversed.has_value() >> fatal);
        expect(reversed->normal == -vertex->normal);
    };

    "the contact point is the middle of the overlap, inside a platform thinner than it"_test = [] {
        // A 0.1 m platform; a circle reaching 0.1 below it: along the normal the circle
        // covers [-0.6, 0.6] from its center, the platform [-0.5, -0.4].
        const Polygon platform = Polygon::fromRectangle({{-2.f, 0.f}, {2.f, 0.1f}});
        const std::optional<Contact> above = collide(platform, Circle{{0.f, 0.5f}, 0.6f});
        expect(above.has_value() >> fatal);
        expect(near(above->point, Vector2f(0.f, 0.05f), Tolerance));

        // Center inside a thicker one: the circle [-0.3, 0.3], the polygon [-0.9, 0.1].
        const Polygon thick = Polygon::fromRectangle({{-2.f, 0.f}, {2.f, 1.f}});
        const std::optional<Contact> inside = collide(thick, Circle{{0.f, 0.9f}, 0.3f});
        expect(inside.has_value() >> fatal);
        expect(inside->normal == Vector2f(0.f, 1.f));
        expect(near(inside->point, Vector2f(0.f, 0.8f), Tolerance));
    };

    "a circle whose center is inside the polygon leaves through the nearest edge"_test = [] {
        // (1.8, 0.5): 0.2 from the right edge, 0.5 from the bottom, 0.92 from the slope.
        const std::optional<Contact> contact = collide(Slope, Circle{{1.8f, 0.5f}, 0.1f});

        expect(contact.has_value() >> fatal);
        expect(contact->normal == Vector2f(1.f, 0.f));
        expect(near(contact->depth, 0.3f, Tolerance));
        expect(contains(Slope, contact->point));
    };

    "touching shapes give a contact of depth 0, apart ones none"_test = [] {
        // A box against the slope's vertical side.
        const std::optional<Contact> touching = collide(Slope, Polygon{{2.f, 0.f}, {3.f, 0.f}, {3.f, 1.f}, {2.f, 1.f}});
        expect(touching.has_value() >> fatal);
        expect(touching->depth == 0.f);
        expect(touching->normal == Vector2f(1.f, 0.f));
        expect(!collide(Slope, Polygon{{2.1f, 0.f}, {3.f, 0.f}, {3.f, 1.f}, {2.1f, 1.f}}).has_value());
    };
}

void test_raycast() {
    "a ray falling on the slope hits it where y = x, with its normal"_test = [] {
        const std::optional<Hit> hit = raycast({1.5f, 5.f}, {0.f, -10.f}, Slope);

        expect(hit.has_value() >> fatal);
        expect(near(hit->point, Vector2f(1.5f, 1.5f), Tolerance));
        expect(near(hit->fraction, 0.35f, Tolerance));
        expect(near(hit->normal, Vector2f(-Diagonal, Diagonal), Tolerance));
    };

    "a ray starting inside, too short, or beside misses"_test = [] {
        expect(!raycast({1.5f, 0.5f}, {0.f, 5.f}, Slope).has_value()) << "inside";
        expect(!raycast({1.5f, 5.f}, {0.f, -3.f}, Slope).has_value()) << "too short";
        expect(!raycast({3.f, 5.f}, {0.f, -10.f}, Slope).has_value()) << "beside";
        expect(!raycast({-1.f, -1.f}, {5.f, 0.f}, Slope).has_value()) << "parallel, below";
    };

    "a ray starting on the surface and going in hits at 0"_test = [] {
        const std::optional<Hit> hit = raycast({1.f, 1.f}, {0.f, -1.f}, Slope);
        expect(hit.has_value() >> fatal);
        expect(hit->fraction == 0.f);
    };

    "a rectangle given as a polygon gets the hit of raycast(origin, delta, Rectangle)"_test = [] {
        const Rectangle box{{-1.f, -1.f}, {1.f, 0.5f}};
        int checked = 0;
        for (float x = -3.f; x <= 3.f; x += 0.5f) {
            for (float y = -3.f; y <= 3.f; y += 0.5f) {
                const Vector2f origin{x, y};
                const Vector2f delta{-2.f * x + 0.25f, -2.f * y};
                const std::optional<Hit> onBox = raycast(origin, delta, box);
                const std::optional<Hit> onPolygon = raycast(origin, delta, Polygon::fromRectangle(box));
                expect(onBox.has_value() == onPolygon.has_value()) << x << y;
                if (onBox && onPolygon) {
                    expect(near(onPolygon->fraction, onBox->fraction, Tolerance)) << x << y;
                    expect(onPolygon->normal == onBox->normal) << x << y;
                    ++checked;
                }
            }
        }
        expect(checked > 20_i);
    };
}

void test_sweep() {
    "a box falling on the slope touches it with its corner, along the slope's normal"_test = [] {
        const Rectangle box = Rectangle::fromCenter({1.5f, 4.f}, {0.25f, 0.25f});
        const std::optional<Hit> hit = sweep(box, {0.f, -4.f}, Slope);

        expect(hit.has_value() >> fatal);
        // The bottom-right corner (1.75, 3.75) reaches the line at y = 1.75: down 2 of 4.
        expect(near(hit->fraction, 0.5f, Tolerance));
        expect(near(hit->point, Vector2f(1.5f, 2.f), Tolerance));
        expect(near(hit->normal, Vector2f(-Diagonal, Diagonal), Tolerance));
    };

    "a rectangle given as a polygon sweeps like sweep(Rectangle, Rectangle)"_test = [] {
        const Rectangle target{{-1.f, -1.f}, {1.f, 0.5f}};
        int checked = 0;
        for (float x = -4.f; x <= 4.f; x += 0.5f) {
            for (float y = -4.f; y <= 4.f; y += 0.5f) {
                const Rectangle moving = Rectangle::fromCenter({x, y}, {0.5f, 0.25f});
                const Vector2f delta{-1.5f * x + 0.25f, -1.5f * y};
                const std::optional<Hit> boxes = sweep(moving, delta, target);
                const std::optional<Hit> polygons = sweep(Polygon::fromRectangle(moving), delta, Polygon::fromRectangle(target));
                expect(boxes.has_value() == polygons.has_value()) << x << y;
                if (boxes && polygons) {
                    expect(near(polygons->fraction, boxes->fraction, Tolerance)) << x << y;
                    expect(polygons->normal == boxes->normal) << x << y;
                    expect(near(polygons->point, boxes->point, Tolerance)) << x << y;
                    ++checked;
                }
            }
        }
        expect(checked > 20_i);
    };

    "a ball rolling down onto the slope's top vertex is turned away from the vertex"_test = [] {
        const std::optional<Hit> hit = sweep(Circle{{2.3f, 5.f}, 0.5f}, {0.f, -5.f}, Slope);

        expect(hit.has_value() >> fatal);
        // Touches when its center is 0.5 from (2, 2): at (2.3, 2.4).
        expect(near(hit->point, Vector2f(2.3f, 2.4f), Tolerance));
        expect(near(hit->normal, Vector2f(0.6f, 0.8f), Tolerance));

        // The slope moving down, away from a ball touching its vertex: no hit.
        expect(!sweep(Slope, {0.f, -5.f}, Circle{{2.3f, 2.4f}, 0.5f}).has_value());
        // Moving up into it: a hit at once, with the normal turned towards the ball.
        const std::optional<Hit> into = sweep(Slope, {0.f, 5.f}, Circle{{2.3f, 2.4f}, 0.5f});
        expect(into.has_value() >> fatal);
        expect(near(into->fraction, 0.f, Tolerance));
        expect(near(into->normal, Vector2f(-0.6f, -0.8f), Tolerance));
    };
}

/** @brief The separation property of a sweep normal: what moves is in front of it, what stays behind. */
std::string separatingFailure(const Vector2f& normal, double movingLowest, double stillHighest, const Vector2d& motion, double tolerance) {
    if (movingLowest < stillHighest - tolerance) {
        return withValue("the normal does not separate the shapes at the hit, overlap", stillHighest - movingLowest);
    }
    return exact(normal).dot(motion) > 0.0 ? "the normal faces the same way as the motion" : std::string();
}

double lowestAlong(const ExactPolygon& polygon, const Vector2d& direction) {
    double lowest = std::numeric_limits<double>::infinity();
    for (const Vector2d& point : polygon.points) {
        lowest = std::min(lowest, point.dot(direction));
    }
    return lowest;
}

void test_random_cases() {
    "on random polygons, overlaps and contains agree with the signed distance"_test = [] {
        Tally polygons, withRectangles, withCircles, inPolygon;
        forRandomCases(RandomCasesPerFlavor, [&](RandomCases& random) {
            const double tolerance = random.tolerance();
            const Vector2f home = random.point(6.0);
            const Polygon a = random.polygon(home);
            const Polygon b = random.polygon(random.near(home, 5.0));
            const Rectangle r = random.rectangle(random.near(home, 5.0));
            const Circle c = random.circle(random.near(home, 5.0));
            const Vector2f point = random.near(home, 4.0);

            polygons.record(checkOverlap(signedDistance(exact(a), exact(b)), overlaps(a, b), tolerance),
                            [&] { return describeCall("overlaps", a, b); });
            withRectangles.record(checkOverlap(signedDistance(exact(a), exact(r)), overlaps(a, r), tolerance),
                                  [&] { return describeCall("overlaps", a, r); });
            withRectangles.record(checkOverlap(signedDistance(exact(r), exact(a)), overlaps(r, a), tolerance),
                                  [&] { return describeCall("overlaps", r, a); });
            withCircles.record(checkOverlap(signedDistance(exact(a), exact(c)), overlaps(a, c), tolerance),
                               [&] { return describeCall("overlaps", a, c); });
            withCircles.record(checkOverlap(signedDistance(exact(c), exact(a)), overlaps(c, a), tolerance),
                               [&] { return describeCall("overlaps", c, a); });
            inPolygon.record(checkOverlap(signedDistance(exact(point), exact(a)), contains(a, point), tolerance),
                             [&] { return describeCall("contains", a, point); });
        });
        for (const Tally* tally : {&polygons, &withRectangles, &withCircles, &inPolygon}) {
            expect(tally->failures == 0_i) << tally->firstFailure;
            expect(tally->yes > 0 && tally->no > 0) << "the random cases must give both answers";
        }
    };

    "on random polygons, collide gives the shortest push that separates them, and a point inside both"_test = [] {
        Tally polygons, polygonRectangle, rectanglePolygon, polygonCircle, circlePolygon;
        forRandomCases(RandomCasesPerFlavor, [&](RandomCases& random) {
            const double tolerance = random.tolerance();
            const Vector2f home = random.point(6.0);
            const Polygon a = random.polygon(home);
            const Polygon b = random.polygon(random.near(home, 4.0));
            const Rectangle r = random.rectangle(random.near(home, 4.0));
            const Circle c = random.circle(random.near(home, 4.0));

            polygons.record(checkContact(exact(a), exact(b), collide(a, b), overlaps(a, b), tolerance),
                            [&] { return describeCall("collide", a, b); });
            polygonRectangle.record(checkContact(exact(a), exact(r), collide(a, r), overlaps(a, r), tolerance),
                                    [&] { return describeCall("collide", a, r); });
            rectanglePolygon.record(checkContact(exact(r), exact(a), collide(r, a), overlaps(r, a), tolerance),
                                    [&] { return describeCall("collide", r, a); });
            polygonCircle.record(checkContact(exact(a), exact(c), collide(a, c), overlaps(a, c), tolerance),
                                 [&] { return describeCall("collide", a, c); });
            circlePolygon.record(checkContact(exact(c), exact(a), collide(c, a), overlaps(c, a), tolerance),
                                 [&] { return describeCall("collide", c, a); });
        });
        for (const Tally* tally : {&polygons, &polygonRectangle, &rectanglePolygon, &polygonCircle, &circlePolygon}) {
            expect(tally->failures == 0_i) << tally->firstFailure;
            expect(tally->yes > 0 && tally->no > 0) << "the random cases must give both answers";
        }
    };

    "on random rays, the hit on a polygon is the first contact, with the edge's normal"_test = [] {
        Tally rays;
        forRandomCases(RandomCasesPerFlavor, [&](RandomCases& random) {
            const double tolerance = random.tolerance();
            const Vector2f home = random.point(6.0);
            const Polygon polygon = random.polygon(home);
            const Vector2f origin = random.near(home, 8.0);
            const Vector2f delta = random.motion();
            const Vector2d start = exact(origin);
            const Vector2d motion = exact(delta);
            const ExactPolygon exactPolygon = exact(polygon);

            const std::optional<Hit> hit = raycast(origin, delta, polygon);
            Verdict verdict = checkFirstContact(
                [&](double t) { return signedDistance(start + motion * t, exactPolygon); }, hit, tolerance);
            if (hit) {
                verdict = alsoCheck(verdict, checkHitPoint(*hit, start, motion, tolerance));
                if (verdict.expected == Expected::Yes) {
                    verdict = alsoCheck(verdict, checkEdgeNormal(hit->normal, exact(hit->point), polygon, motion, tolerance));
                }
            }
            rays.record(verdict, [&] { return describeCall("raycast", origin, delta, polygon); });
        });
        expect(rays.failures == 0_i) << rays.firstFailure;
        expect(rays.yes > 0 && rays.no > 0) << "the random cases must give both answers";
    };

    "on random sweeps, the contact with a polygon is the first one, with a separating normal"_test = [] {
        Tally polygons, rectanglePolygon, polygonRectangle, circlePolygon, polygonCircle;
        forRandomCases(RandomSweepsPerFlavor, [&](RandomCases& random) {
            const double tolerance = random.tolerance();
            const Vector2f home = random.point(6.0);
            const Polygon target = random.polygon(home);
            const Polygon moving = random.polygon(random.near(home, 8.0));
            const Rectangle movingRectangle = random.rectangle(random.near(home, 8.0));
            const Rectangle targetRectangle = random.rectangle(home);
            const Circle movingCircle = random.circle(random.near(home, 8.0));
            const Circle targetCircle = random.circle(home);
            const Vector2f delta = random.motion();
            const Vector2d motion = exact(delta);
            const ExactPolygon exactTarget = exact(target);
            const ExactPolygon exactMoving = exact(moving);

            // Polygon into polygon.
            std::optional<Hit> hit = sweep(moving, delta, target);
            Verdict verdict = checkFirstContact(
                [&](double t) { return signedDistance(exactTarget, moved(exactMoving, motion * t)); }, hit, tolerance);
            if (hit) {
                verdict = alsoCheck(verdict, checkHitPoint(*hit, exact(moving.center()), motion, tolerance));
                if (verdict.expected == Expected::Yes) {
                    const Vector2d n = exact(hit->normal);
                    const ExactPolygon atHit = moved(exactMoving, motion * double(hit->fraction));
                    verdict = alsoCheck(verdict, separatingFailure(hit->normal, lowestAlong(atHit, n), -lowestAlong(exactTarget, -n), motion, tolerance));
                }
            }
            polygons.record(verdict, [&] { return describeCall("sweep", moving, delta, target); });

            // Rectangle into polygon, polygon into rectangle.
            const ExactPolygon exactMovingRectangle = asPolygon(exact(movingRectangle));
            hit = sweep(movingRectangle, delta, target);
            verdict = checkFirstContact(
                [&](double t) { return signedDistance(exactTarget, moved(exactMovingRectangle, motion * t)); }, hit, tolerance);
            if (hit && verdict.expected == Expected::Yes) {
                const Vector2d n = exact(hit->normal);
                const ExactPolygon atHit = moved(exactMovingRectangle, motion * double(hit->fraction));
                verdict = alsoCheck(verdict, separatingFailure(hit->normal, lowestAlong(atHit, n), -lowestAlong(exactTarget, -n), motion, tolerance));
            }
            rectanglePolygon.record(verdict, [&] { return describeCall("sweep", movingRectangle, delta, target); });

            const ExactPolygon exactTargetRectangle = asPolygon(exact(targetRectangle));
            hit = sweep(moving, delta, targetRectangle);
            verdict = checkFirstContact(
                [&](double t) { return signedDistance(exactTargetRectangle, moved(exactMoving, motion * t)); }, hit, tolerance);
            if (hit && verdict.expected == Expected::Yes) {
                const Vector2d n = exact(hit->normal);
                const ExactPolygon atHit = moved(exactMoving, motion * double(hit->fraction));
                verdict = alsoCheck(verdict, separatingFailure(hit->normal, lowestAlong(atHit, n), -lowestAlong(exactTargetRectangle, -n), motion, tolerance));
            }
            polygonRectangle.record(verdict, [&] { return describeCall("sweep", moving, delta, targetRectangle); });

            // Circle into polygon: the normal goes from the polygon's closest point to the center.
            const ExactCircle exactMovingCircle = exact(movingCircle);
            hit = sweep(movingCircle, delta, target);
            verdict = checkFirstContact(
                [&](double t) { return signedDistance(exactTarget, moved(exactMovingCircle, motion * t)); }, hit, tolerance);
            if (hit) {
                verdict = alsoCheck(verdict, checkHitPoint(*hit, exactMovingCircle.center, motion, tolerance));
                if (verdict.expected == Expected::Yes) {
                    const Vector2d n = exact(hit->normal);
                    const Vector2d center = exactMovingCircle.center + motion * double(hit->fraction);
                    verdict = alsoCheck(verdict, separatingFailure(hit->normal, center.dot(n) - exactMovingCircle.radius, -lowestAlong(exactTarget, -n), motion, tolerance));
                }
            }
            circlePolygon.record(verdict, [&] { return describeCall("sweep", movingCircle, delta, target); });

            // Polygon into circle.
            const ExactCircle exactTargetCircle = exact(targetCircle);
            hit = sweep(moving, delta, targetCircle);
            verdict = checkFirstContact(
                [&](double t) { return signedDistance(exactTargetCircle, moved(exactMoving, motion * t)); }, hit, tolerance);
            if (hit && verdict.expected == Expected::Yes) {
                const Vector2d n = exact(hit->normal);
                const ExactPolygon atHit = moved(exactMoving, motion * double(hit->fraction));
                verdict = alsoCheck(verdict, separatingFailure(hit->normal, lowestAlong(atHit, n), exactTargetCircle.center.dot(n) + exactTargetCircle.radius, motion, tolerance));
            }
            polygonCircle.record(verdict, [&] { return describeCall("sweep", moving, delta, targetCircle); });
        });
        for (const Tally* tally : {&polygons, &rectanglePolygon, &polygonRectangle, &circlePolygon, &polygonCircle}) {
            expect(tally->failures == 0_i) << tally->firstFailure;
            expect(tally->yes > 0 && tally->no > 0) << "the random cases must give both answers";
        }
    };
}
}

void run_collision_polygon_tests() {
    test_overlap();
    test_collide();
    test_raycast();
    test_sweep();
    test_random_cases();
}
