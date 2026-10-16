#include <boost/ut.hpp>

#include <cstddef>
#include <vector>

#include "ECS/Core/Resource/ResourceId.hpp"

using namespace boost::ut;

namespace {
    struct Clock {};
    struct Input {};
    struct Settings {};
    struct Audio {};
    struct FirstRequested {};
}

namespace {
void test_resource_id_is_stable_for_same_type() {
    "resourceId is stable for the same type"_test = [] {
        expect(ECS::resourceId<Clock>() == ECS::resourceId<Clock>());
    };
}

void test_resource_ids_are_distinct_for_different_types() {
    "resourceId assigns different ids to different types"_test = [] {
        expect(ECS::resourceId<Clock>() != ECS::resourceId<Input>());
    };
}

void test_resource_id_is_allocated_on_first_call_only() {
    "resourceId consumes one id on the first call only"_test = [] {
        const ECS::ResourceId before = ECS::detail::nextResourceId.load();
        const ECS::ResourceId id = ECS::resourceId<FirstRequested>();
        const ECS::ResourceId afterFirst = ECS::detail::nextResourceId.load();
        (void)ECS::resourceId<FirstRequested>();

        expect(id == before);
        expect(afterFirst == before + 1);
        expect(ECS::detail::nextResourceId.load() == afterFirst);
    };
}

void test_resource_id_is_noexcept() {
    "resourceId is noexcept"_test = [] {
        expect(noexcept(ECS::resourceId<Clock>()));
    };
}

void test_resource_id_distinguishes_const_types() {
    "resourceId gives const and non-const types different ids"_test = [] {
        expect(ECS::resourceId<const Clock>() == ECS::resourceId<const Clock>());
        expect(ECS::resourceId<const Clock>() != ECS::resourceId<Clock>());
    };
}

void test_resource_id_distinguishes_pointer_types() {
    "resourceId gives a type and a pointer to it different ids"_test = [] {
        expect(ECS::resourceId<Clock*>() != ECS::resourceId<Clock>());
    };
}

void test_resource_ids_are_unique_for_many_types() {
    "resourceId remains unique for many resource types"_test = [] {
        const std::vector<ECS::ResourceId> ids{
            ECS::resourceId<Clock>(),
            ECS::resourceId<Input>(),
            ECS::resourceId<Settings>(),
            ECS::resourceId<Audio>()
        };

        for (std::size_t first = 0; first < ids.size(); ++first) {
            for (std::size_t second = first + 1; second < ids.size(); ++second) {
                expect(ids[first] != ids[second]);
            }
        }
    };
}
}

void run_resource_id_tests() {
    test_resource_id_is_stable_for_same_type();
    test_resource_ids_are_distinct_for_different_types();
    test_resource_id_is_allocated_on_first_call_only();
    test_resource_id_is_noexcept();
    test_resource_id_distinguishes_const_types();
    test_resource_id_distinguishes_pointer_types();
    test_resource_ids_are_unique_for_many_types();
}
