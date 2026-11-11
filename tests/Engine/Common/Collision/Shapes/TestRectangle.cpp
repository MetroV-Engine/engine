#include <boost/ut.hpp>

#include "Engine/Common/Collision/Shapes/Rectangle.hpp"

using namespace boost::ut;
using namespace Engine::Collision;

namespace {
void test_construction() {
    "a Rectangle is built from a center and a half size"_test = [] {
        constexpr Rectangle rectangle = Rectangle::fromCenter({2.f, 3.f}, {1.f, 2.f});

        expect(constant<rectangle.min == Vector2f(1.f, 1.f)>);
        expect(constant<rectangle.max == Vector2f(3.f, 5.f)>);
        expect(constant<rectangle.center() == Vector2f(2.f, 3.f)>);
        expect(constant<rectangle.halfSize() == Vector2f(1.f, 2.f)>);
        expect(constant<rectangle.size() == Vector2f(2.f, 4.f)>);
    };
}

void test_transformations() {
    "translated, expanded and merged build new rectangles"_test = [] {
        constexpr Rectangle rectangle{{0.f, 0.f}, {2.f, 1.f}};

        expect(constant<rectangle.translated({1.f, -1.f}) == Rectangle{{1.f, -1.f}, {3.f, 0.f}}>);
        expect(constant<rectangle.expanded({1.f, 0.5f}) == Rectangle{{-1.f, -0.5f}, {3.f, 1.5f}}>);
        expect(constant<rectangle.merged(Rectangle{{-1.f, 0.5f}, {1.f, 4.f}}) == Rectangle{{-1.f, 0.f}, {2.f, 4.f}}>);
    };
}

void test_closest_point() {
    "closestPoint keeps a point inside and clamps one outside"_test = [] {
        constexpr Rectangle rectangle{{0.f, 0.f}, {2.f, 1.f}};

        expect(constant<rectangle.closestPoint({1.f, 0.5f}) == Vector2f(1.f, 0.5f)>);
        expect(constant<rectangle.closestPoint({5.f, -3.f}) == Vector2f(2.f, 0.f)>);
        expect(constant<rectangle.closestPoint({1.f, 7.f}) == Vector2f(1.f, 1.f)>);
    };
}
}

void run_collision_rectangle_tests() {
    test_construction();
    test_transformations();
    test_closest_point();
}
