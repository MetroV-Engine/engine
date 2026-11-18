#include <boost/ut.hpp>

#include "Engine/Common/Math/Scalar.hpp"
#include "../../Support/Approx.hpp"

using namespace boost::ut;
using namespace Engine::Math;
using TestSupport::near;

namespace {
void test_scalar_concept() {
    "Scalar accepts signed integers and floating points only"_test = [] {
        expect(constant<Scalar<int>>);
        expect(constant<Scalar<double>>);
        expect(constant<!Scalar<unsigned>>);
        expect(constant<!Scalar<bool>>);
    };
}

void test_angle_conversions() {
    "toRadians and toDegrees convert between units"_test = [] {
        expect(near(toRadians(180.0), Pi<double>, 1e-12));
        expect(near(toDegrees(Pi<double> / 2), 90.0, 1e-12));
        expect(near(toDegrees(toRadians(37.5)), 37.5, 1e-12));
        expect(near(Tau<double>, 2 * Pi<double>, 1e-12));
    };
}

void test_wrap_angle() {
    "wrapAngle brings an angle into [-pi, pi]"_test = [] {
        expect(near(wrapAngle(toRadians(270.0)), toRadians(-90.0), 1e-12));
        expect(near(wrapAngle(toRadians(-270.0)), toRadians(90.0), 1e-12));
        expect(near(wrapAngle(0.5), 0.5, 1e-12));
        expect(near(std::fabs(wrapAngle(3 * Pi<double>)), Pi<double>, 1e-12));
    };
}

void test_sign() {
    "sign returns -1, 0 or 1"_test = [] {
        expect(constant<sign(-7) == -1>);
        expect(constant<sign(0) == 0>);
        expect(constant<sign(3) == 1>);
        expect(sign(-0.5) == -1.0_d);
        expect(sign(2.5f) == 1.0_f);
    };
}

void test_approx_equal() {
    "approxEqual compares within a tolerance"_test = [] {
        expect(approxEqual(1.0, 1.0 + 1e-6));
        expect(!approxEqual(1.0, 1.001));
        expect(approxEqual(1.0, 1.001, 0.01));
        expect(approxEqual(-2.0, -2.0 - 5e-6));
    };
}

void test_lerp_family() {
    "lerp, inverseLerp and remap agree with each other"_test = [] {
        expect(constant<lerp(2.0, 4.0, 0.5) == 3.0>);
        expect(constant<lerp(2.0, 4.0, 1.5) == 5.0>);
        expect(constant<inverseLerp(2.0, 4.0, 3.0) == 0.5>);
        expect(constant<remap(5.0, 0.0, 10.0, 100.0, 200.0) == 150.0>);
    };

    "inverseLerp of an empty range returns 0"_test = [] {
        expect(inverseLerp(1.0, 1.0, 5.0) == 0.0_d);
    };
}

void test_move_towards() {
    "moveTowards moves by at most maxDelta without overshooting"_test = [] {
        expect(constant<moveTowards(0.0, 10.0, 3.0) == 3.0>);
        expect(constant<moveTowards(0.0, -10.0, 3.0) == -3.0>);
        expect(constant<moveTowards(9.0, 10.0, 3.0) == 10.0>);
        expect(constant<moveTowards(5.0, 5.0, 3.0) == 5.0>);
    };
}

void test_damp_is_frame_rate_independent() {
    "damp covers the same distance whatever the step length"_test = [] {
        double current = 0.0;
        for (int step = 0; step < 60; ++step) {
            current = damp(current, 10.0, 3.0, 1.0 / 60.0);
        }

        expect(near(current, damp(0.0, 10.0, 3.0, 1.0), 1e-9));
        expect(near(current, 10.0 * (1.0 - std::exp(-3.0)), 1e-9));
    };

    "dampFactor is 0 for no time and tends to 1"_test = [] {
        expect(dampFactor(3.0, 0.0) == 0.0_d);
        expect(near(dampFactor(3.0, 100.0), 1.0, 1e-12));
    };
}
}

void run_scalar_tests() {
    test_scalar_concept();
    test_angle_conversions();
    test_wrap_angle();
    test_sign();
    test_approx_equal();
    test_lerp_family();
    test_move_towards();
    test_damp_is_frame_rate_independent();
}
