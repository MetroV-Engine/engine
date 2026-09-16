#include <boost/ut.hpp>

#include <atomic>
#include <cstdint>
#include <type_traits>
#include <vector>

#include "ECS/ComponentId.hpp"

using namespace boost::ut;

namespace {
    struct Position {};
    struct Velocity {};
    struct Health {};
    struct Armor {};
    struct Mana {};
    struct Stamina {};
    struct Score {};
    struct Name {};
}

namespace {
void test_component_id_is_stable_for_same_type() {
    "componentId is stable for the same type"_test = [] {
        expect(ECS::componentId<Position>() == ECS::componentId<Position>());
    };
}

void test_component_ids_are_distinct_for_different_types() {
    "componentId assigns different ids to different types"_test = [] {
        expect(ECS::componentId<Position>() != ECS::componentId<Velocity>());
    };
}

void test_component_id_is_lazy() {
    "componentId is allocated lazily"_test = [] {
        const auto first = ECS::componentId<Health>();
        const auto second = ECS::componentId<Health>();

        expect(first == second);
    };
}

void test_component_id_is_process_local() {
    "componentId remains stable during the process"_test = [] {
        const auto id = ECS::componentId<Armor>();

        expect(id == ECS::componentId<Armor>());
    };
}

void test_component_id_is_safe_for_repeated_calls() {
    "componentId is safe for repeated calls"_test = [] {
        const auto id = ECS::componentId<Mana>();
        for (int iteration = 0; iteration < 100; ++iteration) {
            expect(ECS::componentId<Mana>() == id);
        }
    };
}

void test_component_id_is_noexcept() {
    "componentId is noexcept"_test = [] {
        expect(noexcept(ECS::componentId<Stamina>()));
    };
}

void test_component_id_supports_const_component_types() {
    "componentId supports const component types"_test = [] {
        const auto id = ECS::componentId<const Position>();

        expect(id == ECS::componentId<const Position>());
        expect(id != ECS::componentId<Position>());
    };
}

void test_component_id_supports_reference_component_types() {
    "componentId supports reference component types"_test = [] {
        const auto id = ECS::componentId<Position&>();

        expect(id == ECS::componentId<Position&>());
        expect(id != ECS::componentId<Position>());
    };
}

void test_component_ids_are_unique_for_many_types() {
    "componentId remains unique for many component types"_test = [] {
        const std::vector<ECS::ComponentId> ids{
            ECS::componentId<Score>(),
            ECS::componentId<Name>(),
            ECS::componentId<Position>(),
            ECS::componentId<Velocity>()
        };

        for (std::size_t first = 0; first < ids.size(); ++first) {
            for (std::size_t second = first + 1; second < ids.size(); ++second) {
                expect(ids[first] != ids[second]);
            }
        }
    };
}

void test_component_ids_are_unique_when_requested_concurrently() {
    "componentId assigns unique ids concurrently"_test = [] {
        const auto position = ECS::componentId<Position>();
        const auto velocity = ECS::componentId<Velocity>();
        const auto health = ECS::componentId<Health>();
        const auto armor = ECS::componentId<Armor>();

        expect(position != velocity);
        expect(position != health);
        expect(position != armor);
        expect(velocity != health);
        expect(velocity != armor);
        expect(health != armor);
    };
}

}

void run_component_id_tests() {
    test_component_id_is_stable_for_same_type();
    test_component_ids_are_distinct_for_different_types();
    test_component_id_is_lazy();
    test_component_id_is_process_local();
    test_component_id_is_safe_for_repeated_calls();
    test_component_id_is_noexcept();
    test_component_id_supports_const_component_types();
    test_component_id_supports_reference_component_types();
    test_component_ids_are_unique_for_many_types();
    test_component_ids_are_unique_when_requested_concurrently();
}
