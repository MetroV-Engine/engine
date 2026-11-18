#include <boost/ut.hpp>

#include "Engine/Common/Physic/Forces/Radial.hpp"
#include "../../../Support/Approx.hpp"

using namespace boost::ut;
using namespace Engine;
using TestSupport::near;
using V = Math::Vector2d;

namespace {
void test_falloff() {
    "the push fades linearly from the center to the radius"_test = [] {
        expect(near(Physic::radialPush(V(2, 0), V(0, 0), 100, 4), V(50, 0), 1e-12));
        expect(near(Physic::radialPush(V(0, -1), V(0, 0), 100, 4), V(0, -75), 1e-12));
        expect(near(Physic::radialPush(V(4, 3), V(1, -1), 100, 10), V(0.6, 0.8) * 50, 1e-12));
    };

    "nothing is pushed at the radius or beyond"_test = [] {
        expect(Physic::radialPush(V(4, 0), V(0, 0), 100, 4) == V::zero());
        expect(Physic::radialPush(V(5, 0), V(0, 0), 100, 4) == V::zero());
    };

    "a body on the center is not pushed"_test = [] {
        expect(Physic::radialPush(V(0, 0), V(0, 0), 100, 4) == V::zero());
    };
}

void test_negative_strength() {
    "a negative strength pulls towards the center"_test = [] {
        expect(near(Physic::radialPush(V(2, 0), V(0, 0), -100, 4), V(-50, 0), 1e-12));
    };
}
}

void run_radial_tests() {
    test_falloff();
    test_negative_strength();
}
