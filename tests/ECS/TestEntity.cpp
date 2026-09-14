#include <boost/ut.hpp>

#include <compare>
#include <cstddef>

#include "ECS/Entity.hpp"

namespace {
    using ECS::Entity;
    using ECS::EntityGeneration;
    using namespace boost::ut;

    void test_default_constructor() {
        "default constructor initializes the identity to zero"_test = [] {
            const Entity entity;

            expect(entity.value() == std::size_t{0});
            expect(entity.index() == std::size_t{0});
            expect(entity.generation() == EntityGeneration{0});
        };
    }

    void test_id_constructor() {
        "id constructor stores the identity"_test = [] {
            const Entity entity{42};

            expect(entity.value() == std::size_t{42});
            expect(entity.index() == std::size_t{42});
            expect(entity.generation() == EntityGeneration{0});
        };
    }

    void test_index_generation_constructor() {
        "index and generation constructor stores both values"_test = [] {
            const Entity entity{42, EntityGeneration{7}};

            expect(entity.value() == std::size_t{42});
            expect(entity.index() == std::size_t{42});
            expect(entity.generation() == EntityGeneration{7});
        };
    }

    void test_conversion_operator() {
        "conversion operator returns the index"_test = [] {
            const Entity entity{42, EntityGeneration{7}};

            const std::size_t identity = entity;

            expect(identity == std::size_t{42});
        };
    }

    void test_value() {
        "value returns the index"_test = [] {
            const Entity entity{42, EntityGeneration{7}};

            expect(entity.value() == std::size_t{42});
        };
    }

    void test_index() {
        "index returns the slot index"_test = [] {
            const Entity entity{42, EntityGeneration{7}};

            expect(entity.index() == std::size_t{42});
        };
    }

    void test_generation() {
        "generation returns the slot generation"_test = [] {
            const Entity entity{42, EntityGeneration{7}};

            expect(entity.generation() == EntityGeneration{7});
        };
    }

    void test_strong_ordering_equality() {
        "strong ordering compares equal entities"_test = [] {
            const Entity first{42, EntityGeneration{7}};
            const Entity second{42, EntityGeneration{7}};

            expect((first <=> second) == std::strong_ordering::equal);
            expect(first == second);
        };
    }

    void test_strong_ordering_index() {
        "strong ordering compares indexes before generations"_test = [] {
            const Entity lower{41, EntityGeneration{99}};
            const Entity higher{42, EntityGeneration{0}};

            expect((lower <=> higher) == std::strong_ordering::less);
            expect(lower < higher);
            expect(higher > lower);
        };
    }

    void test_strong_ordering_generation() {
        "strong ordering compares generations for the same index"_test = [] {
            const Entity lower{42, EntityGeneration{6}};
            const Entity higher{42, EntityGeneration{7}};

            expect((lower <=> higher) == std::strong_ordering::less);
            expect(lower < higher);
            expect(higher > lower);
        };
    }
}

int main() {
    test_default_constructor();
    test_id_constructor();
    test_index_generation_constructor();
    test_conversion_operator();
    test_value();
    test_index();
    test_generation();
    test_strong_ordering_equality();
    test_strong_ordering_index();
    test_strong_ordering_generation();
}