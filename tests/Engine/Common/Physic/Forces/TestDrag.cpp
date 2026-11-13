#include <boost/ut.hpp>

#include <cmath>

#include "Engine/Common/Physic/Forces/Drag.hpp"
#include "../../../Support/Approx.hpp"

using namespace boost::ut;
using namespace Engine;
using TestSupport::near;
using V = Math::Vector2d;

namespace {
void test_drag() {
    "linearDrag is -c v, quadraticDrag is -c |v| v: against the motion"_test = [] {
        expect(near(Physic::linearDrag(V(3, -4), 2.0), V(-6, 8), 1e-12));
        expect(near(Physic::quadraticDrag(V(3, -4), 2.0), V(-30, 40), 1e-12));
        expect(Physic::quadraticDrag(V(0, 0), 2.0) == V::zero());
    };

    "linearDragForStep integrates to exactly v e^(-c dt / m), whatever the step"_test = [] {
        for (const double dt : {1.0 / 60.0, 0.5, 10.0}) {
            const V velocity(4, -2);
            const V force = Physic::linearDragForStep(velocity, 3.0, 2.0, dt);
            const V after = velocity + force * (dt / 2.0);
            expect(near(after, velocity * std::exp(-3.0 * dt / 2.0), 1e-12)) << "dt" << dt;
            expect(after.dot(velocity) >= 0.0) << "never reversed";
        }
    };

    "quadraticDragForStep is quadraticDrag, until it would stop the body: then just that"_test = [] {
        const V slow(1, 0);
        expect(near(Physic::quadraticDragForStep(slow, 0.5, 2.0, 1.0 / 60.0), V(-0.5, 0), 1e-12));
        // c v² = 100 * 100 N would remove far more than 10 m/s: only the 10 m/s are removed.
        const V fast(10, 0);
        const V force = Physic::quadraticDragForStep(fast, 100.0, 1.0, 1.0 / 60.0);
        expect(near(fast + force * (1.0 / 60.0), V::zero(), 1e-9));
    };

    "a static body or a step of length 0 gets no drag"_test = [] {
        expect(Physic::linearDragForStep(V(1, 1), 2.0, 0.0, 1.0) == V::zero());
        expect(Physic::linearDragForStep(V(1, 1), 2.0, 1.0, 0.0) == V::zero());
        expect(Physic::quadraticDragForStep(V(1, 1), 2.0, 0.0, 1.0) == V::zero());
        expect(Physic::quadraticDragForStep(V(0, 0), 2.0, 1.0, 1.0) == V::zero());
    };
}
}

void run_drag_tests() {
    test_drag();
}
