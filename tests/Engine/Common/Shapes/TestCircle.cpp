#include <boost/ut.hpp>

#include "Engine/Common/Shapes/Circle.hpp"

using namespace boost::ut;
using namespace Engine::Shapes;

namespace {
void test_bounding_box() {
    "a circle's bounding box is the square around it"_test = [] {
        constexpr Circle circle{{1.f, 2.f}, 0.5f};

        expect(constant<circle.boundingBox() == Rectangle{{0.5f, 1.5f}, {1.5f, 2.5f}}>);
    };

    "a circle of radius 0 has a point as bounding box"_test = [] {
        constexpr Circle point{{3.f, -1.f}, 0.f};

        expect(constant<point.boundingBox() == Rectangle{{3.f, -1.f}, {3.f, -1.f}}>);
    };
}
}

void run_collision_circle_tests() {
    test_bounding_box();
}
