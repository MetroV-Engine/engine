#include <boost/ut.hpp>

#include <sstream>

#include <compare>
#include <cstddef>
#include <type_traits>

#include "ECS/Core/Entity/Entity.hpp"
#include "../../Support/EntityTestAccess.hpp"

using namespace boost::ut;
using ECS::TestAccess::EntityAccess;

void test_default_constructor() {
    "default constructor initializes the identity to zero"_test = [] {
        const ECS::Entity entity;

        expect(entity.index() == std::size_t{0});
        expect(entity.index() == std::size_t{0});
        expect(entity.generation() == ECS::EntityGeneration{0});
    };
}

void test_default_constructor_has_invalid_world() {
    "default constructor belongs to no world"_test = [] {
        const ECS::Entity entity;

        expect(entity.world() == ECS::InvalidWorld);
    };
}

void test_constructors_are_private() {
    "only the default constructor is public"_test = [] {
        static_assert(std::is_default_constructible_v<ECS::Entity>);
        static_assert(!std::is_constructible_v<ECS::Entity, std::size_t>);
        static_assert(!std::is_constructible_v<
            ECS::Entity, std::size_t, ECS::EntityGeneration>);
        static_assert(!std::is_constructible_v<
            ECS::Entity, std::size_t, ECS::EntityGeneration, ECS::WorldId>);
        expect(true);
    };
}

void test_index_generation_world_constructor() {
    "index, generation and world constructor stores all values"_test = [] {
        const ECS::Entity entity = EntityAccess::make(42, 7, 3);

        expect(entity.index() == std::size_t{42});
        expect(entity.index() == std::size_t{42});
        expect(entity.generation() == ECS::EntityGeneration{7});
        expect(entity.world() == ECS::WorldId{3});
    };
}

void test_index() {
    "index returns the slot index"_test = [] {
        const ECS::Entity entity = EntityAccess::make(42, 7);

        expect(entity.index() == std::size_t{42});
    };
}

void test_generation() {
    "generation returns the slot generation"_test = [] {
        const ECS::Entity entity = EntityAccess::make(42, 7);

        expect(entity.generation() == ECS::EntityGeneration{7});
    };
}

void test_strong_ordering_equality() {
    "strong ordering compares equal entities"_test = [] {
        const ECS::Entity first = EntityAccess::make(42, 7, 1);
        const ECS::Entity second = EntityAccess::make(42, 7, 1);

        expect((first <=> second) == std::strong_ordering::equal);
        expect(first == second);
    };
}

void test_equality_compares_generation() {
    "equality distinguishes generations of the same index"_test = [] {
        const ECS::Entity first = EntityAccess::make(42, 6);
        const ECS::Entity second = EntityAccess::make(42, 7);

        expect(!(first == second));
        expect(first != second);
    };
}

void test_equality_compares_world() {
    "equality distinguishes handles from different worlds"_test = [] {
        const ECS::Entity first = EntityAccess::make(42, 7, 1);
        const ECS::Entity second = EntityAccess::make(42, 7, 2);

        expect(!(first == second));
        expect(first != second);
    };
}

void test_strong_ordering_world() {
    "strong ordering compares worlds before indexes"_test = [] {
        const ECS::Entity lower = EntityAccess::make(99, 99, 1);
        const ECS::Entity higher = EntityAccess::make(0, 0, 2);

        expect((lower <=> higher) == std::strong_ordering::less);
        expect(lower < higher);
        expect(higher > lower);
    };
}

void test_strong_ordering_index() {
    "strong ordering compares indexes before generations"_test = [] {
        const ECS::Entity lower = EntityAccess::make(41, 99);
        const ECS::Entity higher = EntityAccess::make(42, 0);

        expect((lower <=> higher) == std::strong_ordering::less);
        expect(lower < higher);
        expect(higher > lower);
    };
}

void test_strong_ordering_generation() {
    "strong ordering compares generations for the same index"_test = [] {
        const ECS::Entity lower = EntityAccess::make(42, 6);
        const ECS::Entity higher = EntityAccess::make(42, 7);

        expect((lower <=> higher) == std::strong_ordering::less);
        expect(lower < higher);
        expect(higher > lower);
    };
}

void test_stream_output() {
    "an Entity prints its index and generation"_test = [] {
        std::ostringstream out;
        out << EntityAccess::make(42, 7);

        expect(out.str() == "Entity(42:7)");
    };
}

void test_stream_output_of_default_entity() {
    "a default Entity prints index and generation zero"_test = [] {
        std::ostringstream out;
        out << ECS::Entity{};

        expect(out.str() == "Entity(0:0)");
    };
}

void test_entity_is_not_a_number() {
    "an Entity does not convert implicitly to a number"_test = [] {
        static_assert(!std::is_convertible_v<ECS::Entity, std::size_t>);
        expect(EntityAccess::make(42, 7).index() == std::size_t{42});
    };
}

void run_entity_tests()
{
    test_default_constructor();
    test_default_constructor_has_invalid_world();
    test_constructors_are_private();
    test_index_generation_world_constructor();
    test_index();
    test_generation();
    test_strong_ordering_equality();
    test_equality_compares_generation();
    test_equality_compares_world();
    test_strong_ordering_world();
    test_strong_ordering_index();
    test_strong_ordering_generation();
    test_stream_output();
    test_stream_output_of_default_entity();
    test_entity_is_not_a_number();
}
