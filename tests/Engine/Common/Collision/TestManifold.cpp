#include <boost/ut.hpp>

#include <algorithm>
#include <cmath>
#include <optional>

#include "Engine/Common/Collision/Collision.hpp"
#include "../../Support/Approx.hpp"
#include "../../Support/CollisionOracle.hpp"

using namespace boost::ut;
using namespace Engine::Collision;
using namespace TestSupport;

namespace {
constexpr float Tolerance = 1e-5f;

Manifold pointsOf(const Polygon& a, const Polygon& b) {
    const std::optional<Contact> contact = collide(a, b);
    expect(contact.has_value() >> fatal);
    return contactPoints(a, b, *contact);
}

void test_faces() {
    "a box sunk into the ground touches it at its two bottom corners, each with its depth"_test = [] {
        const Rectangle ground{{-10.f, -1.f}, {10.f, 0.f}};
        const Rectangle box{{1.f, -0.1f}, {2.f, 0.9f}};
        const Contact contact = *collide(ground, box);
        const Manifold manifold = contactPoints(ground, box, contact);

        expect(manifold.count == 2_u);
        const float left = std::min(manifold.points[0].point.x, manifold.points[1].point.x);
        const float right = std::max(manifold.points[0].point.x, manifold.points[1].point.x);
        expect(near(left, 1.f, Tolerance));
        expect(near(right, 2.f, Tolerance));
        for (std::size_t i = 0; i < 2; ++i) {
            expect(near(manifold.points[i].depth, 0.1f, Tolerance));
            expect(near(manifold.points[i].point.y, -0.05f, Tolerance)) << "halfway between the surfaces";
        }
        expect(near(manifold.center(), Vector2f(1.5f, -0.05f), Tolerance));
    };

    "a box hanging over the ground's edge touches it only where the ground is"_test = [] {
        const Rectangle ground{{-10.f, -1.f}, {0.f, 0.f}};
        const Rectangle box{{-0.5f, -0.1f}, {1.5f, 0.9f}};
        const Manifold manifold = contactPoints(ground, box, *collide(ground, box));

        expect(manifold.count == 2_u);
        expect(near(std::max(manifold.points[0].point.x, manifold.points[1].point.x), 0.f, Tolerance)) << "clipped at the edge";
        expect(near(std::min(manifold.points[0].point.x, manifold.points[1].point.x), -0.5f, Tolerance));
    };

    "a tilted box sunk at one corner touches at that corner only, with the corner's depth"_test = [] {
        const Polygon ground = Polygon::fromRectangle({{-10.f, -1.f}, {10.f, 0.f}});
        const Polygon box = Polygon::fromRectangle({{-0.5f, -0.5f}, {0.5f, 0.5f}}).rotated(0.3f).translated({0.f, 0.6f});
        const Manifold manifold = pointsOf(ground, box);
        const float lowest = std::min({box.vertices[0].y, box.vertices[1].y, box.vertices[2].y, box.vertices[3].y});

        expect(manifold.count == 1_u);
        expect(near(manifold.points[0].depth, -lowest, Tolerance));
    };

    "a box on a slope touches it along its bottom face, at two points"_test = [] {
        const float angle = 0.35f;
        const Polygon slope{{-10.f, -10.f * std::tan(angle)}, {10.f, -10.f * std::tan(angle)}, {10.f, 10.f * std::tan(angle)}};
        const Vector2f normal(-std::sin(angle), std::cos(angle));
        const Polygon box = Polygon::fromRectangle({{-0.5f, -0.5f}, {0.5f, 0.5f}}).rotated(angle).translated(normal * 0.49f);
        const Manifold manifold = pointsOf(slope, box);

        expect(manifold.count == 2_u);
        expect(near(manifold.points[0].depth, 0.01f, 1e-4f));
        expect(near(manifold.points[1].depth, 0.01f, 1e-4f));
        expect(near((manifold.points[1].point - manifold.points[0].point).length(), 1.f, 1e-4f)) << "the whole face";
    };

    "a circle touches at one point: collide's"_test = [] {
        const Rectangle ground{{-10.f, -1.f}, {10.f, 0.f}};
        const Circle ball{{0.f, 0.4f}, 0.5f};
        const Contact contact = *collide(ground, ball);
        const Manifold manifold = contactPoints(ground, ball, contact);

        expect(manifold.count == 1_u);
        expect(manifold.points[0].point == contact.point);
        expect(manifold.points[0].depth == contact.depth);
    };
}

void test_random_cases() {
    "on random polygons, every contact point is in the overlap, no deeper than the contact"_test = [] {
        Tally points;
        forRandomCases(2000, [&](RandomCases& random) {
            const double tolerance = random.tolerance();
            const Vector2f home = random.point(6.0);
            const Polygon a = random.polygon(home);
            const Polygon b = random.polygon(random.near(home, 3.0));
            const std::optional<Contact> contact = collide(a, b);
            Verdict verdict{contact ? Expected::Yes : Expected::No, {}};
            if (contact) {
                const Manifold manifold = contactPoints(a, b, *contact);
                const ExactPolygon exactA = exact(a);
                const ExactPolygon exactB = exact(b);
                if (manifold.count < 1 || manifold.count > 2) {
                    verdict.failure = "not one or two points";
                }
                for (std::size_t i = 0; i < manifold.count && verdict.failure.empty(); ++i) {
                    const Vector2d point = exact(manifold.points[i].point);
                    // Halfway between the surfaces: within half the depth of each shape.
                    const double reach = contact->depth / 2.0 + tolerance;
                    if (signedDistance(point, exactA) > reach || signedDistance(point, exactB) > reach) {
                        verdict.failure = withValue("a point outside the overlap, by",
                                                    std::max(signedDistance(point, exactA), signedDistance(point, exactB)));
                    } else if (manifold.points[i].depth > contact->depth + tolerance || manifold.points[i].depth < 0.f) {
                        verdict.failure = withValue("a point deeper than the contact, depth", manifold.points[i].depth);
                    }
                }
            }
            points.record(verdict, [&] { return describeCall("contactPoints", a, b); });
        });
        expect(points.failures == 0_i) << points.firstFailure;
        expect(points.yes > 100_i);
    };
}
}

void run_collision_manifold_tests() {
    test_faces();
    test_random_cases();
}
