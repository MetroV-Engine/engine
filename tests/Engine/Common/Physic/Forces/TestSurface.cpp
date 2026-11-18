#include <boost/ut.hpp>

#include <cmath>
#include <numbers>

#include "Engine/Common/Physic/Forces/Surface.hpp"
#include "../../../Support/Approx.hpp"

using namespace boost::ut;
using namespace Engine;
using TestSupport::near;
using V = Math::Vector2d;

namespace {
void test_components_on_a_slope() {
    "weight on a 30 degree slope splits into m*g*cos and m*g*sin"_test = [] {
        const double angle = std::numbers::pi / 6;
        const V normal(-std::sin(angle), std::cos(angle));
        const V weight(0, -98.1);

        expect(near(Physic::normalForce(weight, normal), 98.1 * std::cos(angle), 1e-9));
        expect(near(Physic::tangentialComponent(weight, normal).length(), 98.1 * std::sin(angle), 1e-9));
    };

    "the two components add back to the vector"_test = [] {
        const V normal = V(1, 2).normalized();
        const V force(3, -7);

        expect(near(Physic::normalComponent(force, normal) + Physic::tangentialComponent(force, normal), force, 1e-12));
    };

    "the tangential component is perpendicular to the normal"_test = [] {
        const V normal = V(-1, 3).normalized();

        expect(near(Physic::tangentialComponent(V(5, 2), normal).dot(normal), 0.0, 1e-12));
    };
}

void test_flat_ground() {
    "on flat ground the normal force is the whole weight"_test = [] {
        expect(constant<Physic::normalForce(V(0, -98.1), V(0, 1)) == 98.1>);
        expect(constant<Physic::tangentialComponent(V(0, -98.1), V(0, 1)) == V(0, 0)>);
    };

    "a force pulling away from the surface gives no normal force"_test = [] {
        expect(constant<Physic::normalForce(V(0, 50), V(0, 1)) == 0.0>);
    };
}
}

void run_surface_tests() {
    test_components_on_a_slope();
    test_flat_ground();
}
