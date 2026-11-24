#include <boost/ut.hpp>

#include "Input/InputMapping/InputMapping.hpp"

using namespace boost::ut;

void run_input_mapping_tests()
{
    "input: nothing gets through when inactive"_test = [] {
        const InputState state = inputFromKeys(false, true, true, true);
        expect(!state.left && !state.right && !state.jumpPressed);
    };

    "input: keys pass through when active"_test = [] {
        const InputState state = inputFromKeys(true, true, false, true);
        expect(state.left && !state.right && state.jumpPressed);
    };
}
