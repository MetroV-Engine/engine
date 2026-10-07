#include <boost/ut.hpp>

#include <map>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "ECS/Reflection.hpp"

using namespace boost::ut;

namespace {
    struct Position { float x; float y; };

    struct Transform {
        Position pos;
        float rotation;
        std::vector<int> tags;
        std::map<std::string, float> weights;
    };

    struct Empty {};

    struct NotReflected { int a; };

    class Secret {
        public:
            explicit Secret(int value) : _value(value) {}
            [[nodiscard]] int value() const { return _value; }

        private:
            friend struct ECS::reflect::Descriptor<Secret>;
            int _value;
    };
}

E_REFLECT(Position)
    E_FIELD(x, float)
    E_FIELD(y, float)
E_END

E_REFLECT(Transform)
    E_FIELD(pos, Position)
    E_FIELD(rotation, float)
    E_FIELD(tags, std::vector<int>)
    E_FIELD(weights, std::map<std::string, float>)
E_END

E_REFLECT(Empty)
E_END

E_REFLECT(Secret)
    E_FIELD(_value, int)
E_END

namespace {
    // Type-only visitor: records field names.
    struct NameCollector {
        std::vector<std::string> names;

        template<typename F, typename Obj>
        void field(std::string_view name, F Obj::*) { names.emplace_back(name); }
    };

    // Instance visitor: counts fields and sums every float it sees, recursively.
    struct FloatSummer {
        int visited = 0;
        float sum = 0.0f;

        template<typename F>
        void field(std::string_view, const F& value) {
            ++visited;
            if constexpr (std::is_same_v<F, float>) {
                sum += value;
            } else if constexpr (ECS::reflect::Reflected<F>) {
                ECS::reflect::forEachField(value, *this);
            }
        }
    };

    // Type-only visitor: checks each member pointer targets the right member.
    struct MemberPointerChecker {
        bool ok = true;

        template<typename F>
        void field(std::string_view name, F Position::* member) {
            ok = ok && std::is_same_v<F, float>;
            if (name == "x") ok = ok && member == &Position::x;
            if (name == "y") ok = ok && member == &Position::y;
        }
    };

    // Instance visitor: checks every reference it receives is const.
    struct ConstChecker {
        bool& allConst;

        template<typename V>
        void field(std::string_view, V&) { allConst = allConst && std::is_const_v<V>; }
    };

    // Instance visitor: overloads on container field types, ignores the rest.
    struct ContainerGrower {
        void field(std::string_view, std::vector<int>& tags) { tags.push_back(4); }
        void field(std::string_view, std::map<std::string, float>& weights) {
            weights["arm"] = 0.25f;
        }
        template<typename F>
        void field(std::string_view, F&) {}
    };

void test_reflected_concept() {
    "Reflected is satisfied only by E_REFLECT types"_test = [] {
        static_assert(ECS::reflect::Reflected<Position>);
        static_assert(ECS::reflect::Reflected<Transform>);
        static_assert(ECS::reflect::Reflected<Empty>);
        static_assert(!ECS::reflect::Reflected<NotReflected>);
        static_assert(!ECS::reflect::Reflected<int>);

        expect(ECS::reflect::Reflected<Position>);
        expect(!ECS::reflect::Reflected<NotReflected>);
    };
}

void test_type_name() {
    "typeName returns the name written in E_REFLECT"_test = [] {
        static_assert(ECS::reflect::typeName<Position>() == "Position");

        expect(ECS::reflect::typeName<Position>() == std::string_view{"Position"});
        expect(ECS::reflect::typeName<Transform>() == std::string_view{"Transform"});
    };
}

void test_type_only_visit_order() {
    "forEachField<T> visits fields in declaration order"_test = [] {
        NameCollector collector;
        ECS::reflect::forEachField<Transform>(collector);

        expect(collector.names ==
               std::vector<std::string>{"pos", "rotation", "tags", "weights"});
    };
}

void test_type_only_visit_gives_member_pointers() {
    "forEachField<T> passes the matching type and member pointer"_test = [] {
        MemberPointerChecker checker;
        ECS::reflect::forEachField<Position>(checker);

        expect(checker.ok);
    };
}

void test_instance_visit_reads_values() {
    "forEachField(obj) passes the current field values"_test = [] {
        const Position position{1.5f, -2.0f};
        std::vector<float> values;

        struct Reader {
            std::vector<float>& out;
            void field(std::string_view, const float& value) { out.push_back(value); }
        };
        ECS::reflect::forEachField(position, Reader{values});

        expect(values == std::vector<float>{1.5f, -2.0f});
    };
}

void test_instance_visit_writes_values() {
    "forEachField(obj) gives mutable references on a non-const object"_test = [] {
        Position position{1.0f, 2.0f};

        struct AddOne {
            void field(std::string_view, float& value) { value += 1.0f; }
        };
        ECS::reflect::forEachField(position, AddOne{});

        expect(position.x == 2.0_f);
        expect(position.y == 3.0_f);
    };
}

void test_instance_visit_targets_one_field() {
    "a visitor can target a single field by name"_test = [] {
        Position position{1.0f, 2.0f};

        struct AddTo {
            std::string_view target;
            float amount;
            void field(std::string_view name, float& value) {
                if (name == target) value += amount;
            }
        };
        ECS::reflect::forEachField(position, AddTo{"y", 5.0f});

        expect(position.x == 1.0_f);
        expect(position.y == 7.0_f);
    };
}

void test_const_instance_gives_const_references() {
    "forEachField on a const object passes const references"_test = [] {
        const Position position{0.0f, 0.0f};
        bool allConst = true;
        ECS::reflect::forEachField(position, ConstChecker{allConst});

        expect(allConst);
    };
}

void test_nested_reflected_types() {
    "a visitor can recurse into nested reflected fields"_test = [] {
        const Transform transform{{1.0f, 2.0f}, 3.0f, {}, {}};
        FloatSummer summer;
        ECS::reflect::forEachField(transform, summer);

        // 4 Transform fields + 2 Position fields.
        expect(summer.visited == 6_i);
        expect(summer.sum == 6.0_f);
    };
}

void test_complex_field_types() {
    "fields with template types containing commas are supported"_test = [] {
        Transform transform{{}, 0.0f, {1, 2, 3}, {{"head", 0.5f}}};

        ECS::reflect::forEachField(transform, ContainerGrower{});

        expect(transform.tags == std::vector<int>{1, 2, 3, 4});
        expect(transform.weights.size() == 2_ul);
        expect(transform.weights.at("arm") == 0.25_f);
    };
}

void test_empty_type_has_no_fields() {
    "a reflected type without E_FIELD visits nothing"_test = [] {
        NameCollector collector;
        ECS::reflect::forEachField<Empty>(collector);
        Empty empty;
        FloatSummer summer;
        ECS::reflect::forEachField(empty, summer);

        expect(collector.names.empty());
        expect(summer.visited == 0_i);
    };
}

void test_lvalue_visitor_keeps_state() {
    "an lvalue visitor keeps its state after the visit"_test = [] {
        NameCollector collector;
        ECS::reflect::forEachField<Position>(collector);
        ECS::reflect::forEachField<Position>(collector);

        expect(collector.names.size() == 4_ul);
    };
}

void test_private_fields_with_friend() {
    "private fields are reachable when the type befriends its Descriptor"_test = [] {
        Secret secret{41};

        struct Increment {
            void field(std::string_view, int& value) { ++value; }
        };
        ECS::reflect::forEachField(secret, Increment{});

        expect(secret.value() == 42_i);
    };
}

}

void run_reflection_tests() {
    test_reflected_concept();
    test_type_name();
    test_type_only_visit_order();
    test_type_only_visit_gives_member_pointers();
    test_instance_visit_reads_values();
    test_instance_visit_writes_values();
    test_instance_visit_targets_one_field();
    test_const_instance_gives_const_references();
    test_nested_reflected_types();
    test_complex_field_types();
    test_empty_type_has_no_fields();
    test_lvalue_visitor_keeps_state();
    test_private_fields_with_friend();
}
