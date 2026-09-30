#include <boost/ut.hpp>

#include <cstddef>
#include <stdexcept>

#include "ECS/Signature.hpp"

using namespace boost::ut;

namespace {
    void test_max_component_types_is_256() {
        "MaxComponentTypes is 256"_test = [] {
            expect(ECS::MaxComponentTypes == std::size_t{256});
        };
    }

    void test_signature_is_empty_by_default() {
        "Signature is empty by default"_test = [] {
            const ECS::Signature signature;

            expect(signature.none());
            expect(signature.count() == std::size_t{0});
        };
    }

    void test_signature_set_marks_component_present() {
        "Signature set marks a component present"_test = [] {
            ECS::Signature signature;
            signature.set(4);

            expect(signature.test(4));
        };
    }

    void test_signature_reset_removes_component() {
        "Signature reset removes a component"_test = [] {
            ECS::Signature signature;
            signature.set(4);
            signature.reset(4);

            expect(!signature.test(4));
            expect(signature.none());
        };
    }

    void test_signature_supports_multiple_components() {
        "Signature supports multiple components"_test = [] {
            ECS::Signature signature;
            signature.set(1);
            signature.set(7);
            signature.set(42);

            expect(signature.test(1));
            expect(signature.test(7));
            expect(signature.test(42));
            expect(signature.count() == std::size_t{3});
        };
    }

    void test_signature_bitwise_and_matches_common_components() {
        "Signature bitwise and keeps common components"_test = [] {
            ECS::Signature first;
            ECS::Signature second;
            first.set(1);
            first.set(2);
            second.set(2);
            second.set(3);

            const ECS::Signature common = first & second;

            expect(common.count() == std::size_t{1});
            expect(common.test(2));
        };
    }

    void test_signature_equality_compares_all_component_bits() {
        "Signature equality compares all component bits"_test = [] {
            ECS::Signature first;
            ECS::Signature second;
            first.set(10);
            second.set(10);

            expect(first == second);
        };
    }

    void test_signature_set_throws_for_out_of_range_component_id() {
        "Signature set throws for an out of range component id"_test = [] {
            ECS::Signature signature;

            expect(throws<std::out_of_range>([&] {
                signature.set(ECS::MaxComponentTypes);
            }));
        };
    }

    void test_signature_supports_last_valid_component_id() {
        "Signature supports the last valid component id"_test = [] {
            ECS::Signature signature;
            signature.set(ECS::MaxComponentTypes - 1);

            expect(signature.test(ECS::MaxComponentTypes - 1));
        };
    }
}

void run_signature_tests() {
    test_max_component_types_is_256();
    test_signature_is_empty_by_default();
    test_signature_set_marks_component_present();
    test_signature_reset_removes_component();
    test_signature_supports_multiple_components();
    test_signature_bitwise_and_matches_common_components();
    test_signature_equality_compares_all_component_bits();
    test_signature_set_throws_for_out_of_range_component_id();
    test_signature_supports_last_valid_component_id();
}
