#include <boost/ut.hpp>

#include "Support/Expect.hpp"

#include <cmath>

using namespace boost::ut;

void run_expect_helper_tests()
{
    "expect helpers: floatEq accepts 4 ULPs and refuses 5"_test = [] {
        const float one = 1.0f;
        float four = one;
        for (int i = 0; i < 4; ++i)
            four = std::nextafter(four, 2.0f);
        const float five = std::nextafter(four, 2.0f);
        expect(test::floatEq(one, four));
        expect(!test::floatEq(one, five));
        expect(test::floatEq(0.0f, -0.0f));
        expect(!test::floatEq(std::nanf(""), std::nanf("")));
    };

    "expect helpers: near is inclusive"_test = [] {
        expect(test::near(10, 11, 1));
        expect(!test::near(10, 12, 1));
        expect(test::near(0.5f, 0.75f, 0.25f));
    };
}
