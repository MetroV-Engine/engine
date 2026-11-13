#include <boost/ut.hpp>

#include <cmath>
#include <vector>

#include "Engine/Common/Physic/Collision/Collide.hpp"
#include "Engine/Common/Physic/Collision/Overlap.hpp"
#include "../../../Support/Approx.hpp"
#include "../../../Support/CollisionOracle.hpp"

using namespace boost::ut;
using namespace Engine::Collision;
using namespace TestSupport;

namespace {
constexpr float Tolerance = 1e-5f;
constexpr int RandomCasesPerFlavor = 5000;

// depth must be exactly the distance to separate along normal: moved a little
// less, b still overlaps a; moved a little more, it no longer does. (Moved by
// exactly depth, float rounding may leave a 1e-7 gap or overlap either way.)
template <typename A, typename B, typename Move>
bool separatesExactly(const A& a, const B& b, Move moveB) {
    const std::optional<Contact> contact = collide(a, b);
    if (!contact) {
        return false;
    }
    const B almost = moveB(b, contact->normal * (contact->depth - Tolerance));
    const B beyond = moveB(b, contact->normal * (contact->depth + Tolerance));
    return overlaps(a, almost) && !overlaps(a, beyond);
}

const auto moveBox = [](const Rectangle& box, const Vector2f& offset) { return box.translated(offset); };
const auto moveCircle = [](const Circle& circle, const Vector2f& offset) {
    return Circle{circle.center + offset, circle.radius};
};

void test_box_box() {
    "boxes overlapping on the right are separated along +X"_test = [] {
        const Rectangle a = Rectangle::fromCenter({0.f, 0.f}, {1.f, 1.f});
        const Rectangle b = Rectangle::fromCenter({1.5f, 0.2f}, {1.f, 1.f});

        const std::optional<Contact> contact = collide(a, b);

        expect(contact.has_value() >> fatal);
        expect(contact->normal == Vector2f(1.f, 0.f));
        expect(near(contact->depth, 0.5f, Tolerance));
        expect(near(contact->point, Vector2f(0.75f, 0.1f), Tolerance));
    };

    "a box landing on another is separated along +Y"_test = [] {
        const Rectangle a = Rectangle::fromCenter({0.f, 0.f}, {1.f, 1.f});
        const Rectangle b = Rectangle::fromCenter({0.3f, 1.8f}, {1.f, 1.f});

        const std::optional<Contact> contact = collide(a, b);

        expect(contact.has_value() >> fatal);
        expect(contact->normal == Vector2f(0.f, 1.f));
        expect(near(contact->depth, 0.2f, Tolerance));
    };

    "a box inside another leaves through the nearest side, not by its own width"_test = [] {
        const Rectangle big{{0.f, 0.f}, {10.f, 10.f}};
        const Rectangle small{{8.f, 4.f}, {9.f, 6.f}};

        const std::optional<Contact> contact = collide(big, small);

        // The overlap is only 1 wide, but the small box needs 2 to get out on the right.
        expect(contact.has_value() >> fatal);
        expect(contact->normal == Vector2f(1.f, 0.f));
        expect(contact->depth == 2.f);
    };

    "touching boxes give a contact of depth 0, separate boxes none"_test = [] {
        const Rectangle a{{0.f, 0.f}, {1.f, 1.f}};

        const std::optional<Contact> touching = collide(a, Rectangle{{1.f, 0.f}, {2.f, 1.f}});
        expect(touching.has_value() >> fatal);
        expect(touching->depth == 0.f);
        expect(touching->normal == Vector2f(1.f, 0.f));

        expect(!collide(a, Rectangle{{1.5f, 0.f}, {2.f, 1.f}}).has_value());
    };

    "a box exactly on a corner is caught by the floor (ties go to Y)"_test = [] {
        const Rectangle a{{0.f, 0.f}, {1.f, 1.f}};
        const Rectangle b{{0.75f, 0.75f}, {2.75f, 2.75f}};

        const std::optional<Contact> contact = collide(a, b);

        expect(contact.has_value() >> fatal);
        expect(contact->normal == Vector2f(0.f, 1.f));
        expect(contact->depth == 0.25f);
    };

    "moving the second box by normal * depth leaves them exactly touching"_test = [] {
        const Rectangle a{{0.f, 0.f}, {4.f, 2.f}};
        const std::vector<Rectangle> others{
            Rectangle::fromCenter({3.5f, 1.f}, {1.f, 1.f}), Rectangle::fromCenter({-0.5f, 1.5f}, {1.f, 0.5f}),
            Rectangle::fromCenter({2.f, 2.4f}, {3.f, 0.5f}), Rectangle::fromCenter({1.f, 0.2f}, {0.3f, 0.3f}),
            Rectangle::fromCenter({2.f, 1.f}, {2.f, 1.f})};
        for (const Rectangle& b : others) {
            expect(separatesExactly(a, b, moveBox));
        }
    };
}

void test_circle_circle() {
    "overlapping circles are separated along the line between their centers"_test = [] {
        const std::optional<Contact> contact = collide(Circle{{0.f, 0.f}, 1.f}, Circle{{1.5f, 0.f}, 1.f});

        expect(contact.has_value() >> fatal);
        expect(near(contact->normal, Vector2f(1.f, 0.f), Tolerance));
        expect(near(contact->depth, 0.5f, Tolerance));
        expect(near(contact->point, Vector2f(0.75f, 0.f), Tolerance));
    };

    "concentric circles are pushed apart along +Y"_test = [] {
        const std::optional<Contact> contact = collide(Circle{{1.f, 1.f}, 1.f}, Circle{{1.f, 1.f}, 0.5f});

        expect(contact.has_value() >> fatal);
        expect(contact->normal == Vector2f(0.f, 1.f));
        expect(contact->depth == 1.5f);
    };

    "touching circles give depth 0, separate ones none"_test = [] {
        const std::optional<Contact> touching = collide(Circle{{0.f, 0.f}, 1.f}, Circle{{2.f, 0.f}, 1.f});
        expect(touching.has_value() >> fatal);
        expect(touching->depth == 0.f);

        expect(!collide(Circle{{0.f, 0.f}, 1.f}, Circle{{2.1f, 0.f}, 1.f}).has_value());
    };

    "a small circle deep inside a big one gets a contact point inside both"_test = [] {
        const Circle big{{0.f, 0.f}, 3.f};
        const Circle small{{1.f, 0.f}, 0.2f};

        const std::optional<Contact> contact = collide(big, small);

        // The overlap is the small circle itself: its middle is the small circle's center.
        expect(contact.has_value() >> fatal);
        expect(near(contact->depth, 2.2f, Tolerance));
        expect(near(contact->point, small.center, Tolerance));
    };

    "moving the second circle by normal * depth leaves them exactly touching"_test = [] {
        const Circle a{{0.f, 0.f}, 1.f};
        for (const Circle& b : {Circle{{1.f, 1.f}, 1.f}, Circle{{-0.3f, 0.2f}, 0.5f}, Circle{{0.f, -1.5f}, 2.f}}) {
            expect(separatesExactly(a, b, moveCircle));
        }
    };
}

void test_box_circle() {
    "a circle against a face is pushed straight out of it"_test = [] {
        const Rectangle box{{-1.f, -1.f}, {1.f, 1.f}};

        const std::optional<Contact> contact = collide(box, Circle{{1.5f, 0.f}, 1.f});

        expect(contact.has_value() >> fatal);
        expect(contact->normal == Vector2f(1.f, 0.f));
        expect(near(contact->depth, 0.5f, Tolerance));
        expect(near(contact->point, Vector2f(0.75f, 0.f), Tolerance));
    };

    "a circle against a corner is pushed away from the corner"_test = [] {
        const Rectangle box{{-1.f, -1.f}, {1.f, 1.f}};

        const std::optional<Contact> contact = collide(box, Circle{{1.5f, 1.5f}, 1.f});

        const float diagonal = std::sqrt(0.5f);
        expect(contact.has_value() >> fatal);
        expect(near(contact->normal, Vector2f(diagonal, diagonal), Tolerance));
        expect(near(contact->depth, 1.f - std::sqrt(0.5f), Tolerance));
    };

    "a circle whose center is inside the box leaves through the nearest face"_test = [] {
        const Rectangle box{{-1.f, -1.f}, {1.f, 1.f}};

        const std::optional<Contact> contact = collide(box, Circle{{0.8f, 0.f}, 0.5f});

        expect(contact.has_value() >> fatal);
        expect(contact->normal == Vector2f(1.f, 0.f));
        expect(near(contact->depth, 0.7f, Tolerance));
        expect(near(contact->point, Vector2f(0.65f, 0.f), Tolerance));
    };

    "collide(circle, box) is collide(box, circle) with the normal turned around"_test = [] {
        const Rectangle box{{-1.f, -1.f}, {1.f, 1.f}};
        const Circle circle{{1.5f, 0.3f}, 1.f};

        const std::optional<Contact> boxFirst = collide(box, circle);
        const std::optional<Contact> circleFirst = collide(circle, box);

        expect((boxFirst.has_value() && circleFirst.has_value()) >> fatal);
        expect(circleFirst->normal == -boxFirst->normal);
        expect(circleFirst->depth == boxFirst->depth);
        expect(!collide(Circle{{5.f, 0.f}, 1.f}, box).has_value());
    };

    "the contact point stays inside a platform thinner than the overlap"_test = [] {
        const Rectangle platform{{-2.f, 0.f}, {2.f, 0.1f}};

        const std::optional<Contact> contact = collide(platform, Circle{{0.f, 1.f}, 1.5f});

        // The circle is 0.6 deep, but the platform is only 0.1 thick: the overlap is that thickness.
        expect(contact.has_value() >> fatal);
        expect(near(contact->depth, 0.6f, Tolerance));
        expect(near(contact->point, Vector2f(0.f, 0.05f), Tolerance));
    };

    "the contact point of a small circle all inside a box is the circle's center"_test = [] {
        const Rectangle room{{0.f, 0.f}, {10.f, 10.f}};
        const Circle pebble{{5.f, 9.f}, 0.2f};

        const std::optional<Contact> contact = collide(room, pebble);

        expect(contact.has_value() >> fatal);
        expect(contact->normal == Vector2f(0.f, 1.f));
        expect(near(contact->depth, 1.2f, Tolerance));
        expect(near(contact->point, pebble.center, Tolerance));
    };

    "moving the circle by normal * depth leaves it exactly touching the box"_test = [] {
        const Rectangle box{{-1.f, -1.f}, {1.f, 1.f}};
        for (const Circle& circle : {Circle{{1.5f, 0.2f}, 1.f}, Circle{{1.3f, 1.4f}, 0.6f},
                                     Circle{{0.8f, 0.f}, 0.5f}, Circle{{0.f, -0.7f}, 0.4f}}) {
            expect(separatesExactly(box, circle, moveCircle));
        }
    };
}

void test_agrees_with_overlaps() {
    "collide finds a contact exactly when overlaps says the shapes overlap"_test = [] {
        const Rectangle box{{-1.f, -1.f}, {1.f, 1.f}};
        int checked = 0;
        for (float x = -3.f; x <= 3.f; x += 0.25f) {
            for (float y = -3.f; y <= 3.f; y += 0.25f) {
                const Circle circle{{x, y}, 0.8f};
                const Rectangle other = Rectangle::fromCenter({x, y}, {0.6f, 0.4f});
                expect(collide(box, circle).has_value() == overlaps(box, circle));
                expect(collide(box, other).has_value() == overlaps(box, other));
                expect(collide(Circle{{0.f, 0.f}, 1.f}, circle).has_value() == overlaps(Circle{{0.f, 0.f}, 1.f}, circle));
                ++checked;
            }
        }
        expect(checked == 625_i);
    };

    "a circle against a side gets exactly that side's normal"_test = [] {
        const Rectangle box{{-1.f, -1.f}, {1.f, 1.f}};
        int sides = 0;
        for (float x = -2.5f; x <= 2.5f; x += 0.1f) {
            for (float y = -2.5f; y <= 2.5f; y += 0.1f) {
                const Circle circle{{x, y}, 1.6f};
                const bool besideX = std::fabs(y) <= 1.f && std::fabs(x) > 1.f;
                const bool besideY = std::fabs(x) <= 1.f && std::fabs(y) > 1.f;
                const std::optional<Contact> contact = collide(box, circle);
                if (!contact || !(besideX || besideY)) {
                    continue;
                }
                const Vector2f side = besideX ? Vector2f(x > 0.f ? 1.f : -1.f, 0.f) : Vector2f(0.f, y > 0.f ? 1.f : -1.f);
                expect(contact->normal == side) << "circle at" << x << y;
                ++sides;
            }
        }
        expect(sides > 100_i);
    };
}

void test_random_cases() {
    "on random shapes, collide gives the shortest push that separates them"_test = [] {
        Tally rectangles, circles, rectangleCircle, circleRectangle;
        forRandomCases(RandomCasesPerFlavor, [&](RandomCases& random) {
            const double tolerance = random.tolerance();
            const Vector2f home = random.point(6.0);
            const Rectangle a = random.rectangle(home);
            const Rectangle b = random.rectangle(random.near(home, 5.0));
            const Circle c = random.circle(home);
            const Circle d = random.circle(random.near(home, 5.0));

            rectangles.record(checkContact(exact(a), exact(b), collide(a, b), overlaps(a, b), tolerance),
                              [&] { return describeCall("collide", a, b); });
            circles.record(checkContact(exact(c), exact(d), collide(c, d), overlaps(c, d), tolerance),
                           [&] { return describeCall("collide", c, d); });
            rectangleCircle.record(checkContact(exact(a), exact(d), collide(a, d), overlaps(a, d), tolerance),
                                   [&] { return describeCall("collide", a, d); });
            circleRectangle.record(checkContact(exact(c), exact(b), collide(c, b), overlaps(c, b), tolerance),
                                   [&] { return describeCall("collide", c, b); });
        });
        for (const Tally* tally : {&rectangles, &circles, &rectangleCircle, &circleRectangle}) {
            expect(tally->failures == 0_i) << tally->firstFailure;
            expect(tally->yes > 0 && tally->no > 0) << "the random cases must give both answers";
        }
    };
}
}

void run_collision_collide_tests() {
    test_box_box();
    test_circle_circle();
    test_box_circle();
    test_agrees_with_overlaps();
    test_random_cases();
}
