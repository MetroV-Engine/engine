#include <boost/ut.hpp>

#include <cstdint>
#include <new>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "ECS/Ref.hpp"

using namespace boost::ut;

// The registry is a process-wide singleton shared with every other test file:
// type names here are prefixed to never collide with another TU's E_REFLECT.
namespace {
    struct TrVec2 { float x; float y; };

    struct TrBody {
        TrVec2 position;
        double mass;
        std::vector<int> tags;
    };

    struct TrActor {
        std::string name;
        std::int32_t health;
        TrBody body;
    };

    struct TrNoDefault {
        explicit TrNoDefault(int v) : value(v) {}
        int value;
    };

    struct TrDuplicate { int a; };

    namespace other {
        struct TrDuplicate { int b; };
    }

    // Counts live instances to check the TypeInfo lifetime operations.
    struct TrCounted {
        static inline int alive = 0;
        int value = 7;
        TrCounted() { ++alive; }
        TrCounted(const TrCounted& other) : value(other.value) { ++alive; }
        ~TrCounted() { --alive; }
    };
}

E_REFLECT(TrVec2)
    E_FIELD(x, float)
    E_FIELD(y, float)
E_END

E_REFLECT(TrBody)
    E_FIELD(position, TrVec2)
    E_FIELD(mass, double)
    E_FIELD(tags, std::vector<int>)
E_END

E_REFLECT(TrActor)
    E_FIELD(name, std::string)
    E_FIELD(health, std::int32_t)
    E_FIELD(body, TrBody)
E_END

E_REFLECT(TrNoDefault)
    E_FIELD(value, int)
E_END

E_REFLECT(TrDuplicate)
    E_FIELD(a, int)
E_END

E_REFLECT(TrCounted)
    E_FIELD(value, int)
E_END

// Hand-written Descriptor reusing the name "TrDuplicate" for a different type.
template<> struct ECS::reflect::Descriptor<other::TrDuplicate> {
    static constexpr std::string_view name = "TrDuplicate";
    template<typename Visitor> static void forEach(Visitor&) {}
};

using ECS::reflect::Ref;
using ECS::reflect::TypeInfo;
using ECS::reflect::TypeKind;
using ECS::reflect::TypeRegistry;

namespace {
    TypeRegistry& registry() { return TypeRegistry::instance(); }

void test_type_id_is_a_stable_name_hash() {
    "typeId is a constexpr hash of the name"_test = [] {
        static_assert(ECS::reflect::typeId("TrVec2") == ECS::reflect::typeId("TrVec2"));
        static_assert(ECS::reflect::typeId("TrVec2") != ECS::reflect::typeId("TrBody"));

        expect(registry().get<TrVec2>().id == ECS::reflect::typeId("TrVec2"));
    };
}

void test_builtin_scalars() {
    "built-in scalars are pre-registered with portable names"_test = [] {
        const TypeInfo* f = registry().find<float>();
        const TypeInfo* i = registry().find<std::int32_t>();
        const TypeInfo* s = registry().find("string");

        expect(f != nullptr && f->name == "float" && f->kind == TypeKind::Scalar);
        expect(i != nullptr && i->name == "int32");
        expect(s != nullptr && s == registry().find<std::string>());
    };
}

void test_register_is_idempotent() {
    "registerType returns the same TypeInfo every time"_test = [] {
        const TypeInfo& first = registry().registerType<TrVec2>();
        const TypeInfo& second = registry().registerType<TrVec2>();
        const TypeInfo& constQualified = registry().registerType<const TrVec2>();

        expect(&first == &second);
        expect(&first == &constQualified);
    };
}

void test_find_without_registering() {
    "find<T> does not register an unknown type"_test = [] {
        struct NeverRegistered {};

        expect(registry().find<NeverRegistered>() == nullptr);
        expect(registry().find("NeverRegistered") == nullptr);
        expect(registry().find(ECS::reflect::typeId("NeverRegistered")) == nullptr);
    };
}

void test_lookup_by_name_and_id() {
    "a registered type is found by name and by id"_test = [] {
        const TypeInfo& vec = registry().get<TrVec2>();

        expect(registry().find("TrVec2") == &vec);
        expect(registry().find(vec.id) == &vec);
    };
}

void test_struct_fields() {
    "a reflected type gets its fields in order with their types"_test = [] {
        const TypeInfo& body = registry().get<TrBody>();

        expect(body.kind == TypeKind::Struct);
        expect(body.size == sizeof(TrBody));
        expect(body.align == alignof(TrBody));
        expect((body.fields.size() == 3_ul) >> fatal);
        expect(body.fields[0].name == "position");
        expect(body.fields[0].type == &registry().get<TrVec2>());
        expect(body.fields[1].name == "mass");
        expect(body.fields[1].type == registry().find<double>());
        expect(body.fields[2].name == "tags");
    };
}

void test_field_types_are_registered_recursively() {
    "registering a type registers its field types"_test = [] {
        (void)registry().get<TrActor>();

        expect(registry().find("TrBody") != nullptr);
        expect(registry().find("TrVec2") != nullptr);
    };
}

void test_unreflected_field_type_is_opaque() {
    "a field type that is neither scalar nor reflected is Opaque"_test = [] {
        const TypeInfo* tags = registry().get<TrBody>().findField("tags")->type;

        expect(tags->kind == TypeKind::Opaque);
        expect(tags->fields.empty());
        expect(tags->size == sizeof(std::vector<int>));
        expect(tags == registry().find<std::vector<int>>());
    };
}

void test_find_field() {
    "findField returns the field or nullptr"_test = [] {
        const TypeInfo& vec = registry().get<TrVec2>();

        expect(vec.findField("y") == &vec.fields[1]);
        expect(vec.findField("z") == nullptr);
    };
}

void test_field_accessor() {
    "FieldInfo::access returns the address of the member"_test = [] {
        TrBody body{{1.0f, 2.0f}, 3.0, {}};
        const TypeInfo& info = registry().get<TrBody>();

        expect(info.findField("mass")->access(&body) == &body.mass);
        expect(info.findField("position")->access(&body) == &body.position);
    };
}

void test_name_collision_throws() {
    "two different types with the same name are rejected"_test = [] {
        (void)registry().get<TrDuplicate>();

        expect(throws<std::logic_error>([] { (void)registry().get<other::TrDuplicate>(); }));
        expect(registry().find<other::TrDuplicate>() == nullptr);
    };
}

void test_types_lists_registrations() {
    "types() lists every registered type"_test = [] {
        const TypeInfo& vec = registry().get<TrVec2>();
        bool listed = false;
        for (const TypeInfo* type : registry().types()) {
            listed = listed || type == &vec;
        }

        expect(listed);
    };
}

void test_lifetime_operations() {
    "construct, copy and destroy operate on raw storage"_test = [] {
        const TypeInfo& info = registry().get<TrCounted>();
        alignas(TrCounted) std::byte a[sizeof(TrCounted)];
        alignas(TrCounted) std::byte b[sizeof(TrCounted)];

        info.construct(a);
        expect(TrCounted::alive == 1_i);
        expect(std::launder(reinterpret_cast<TrCounted*>(a))->value == 7_i);

        std::launder(reinterpret_cast<TrCounted*>(a))->value = 9;
        info.copy(b, a);
        expect(TrCounted::alive == 2_i);
        expect(std::launder(reinterpret_cast<TrCounted*>(b))->value == 9_i);

        info.destroy(a);
        info.destroy(b);
        expect(TrCounted::alive == 0_i);
    };
}

void test_no_default_constructor() {
    "construct is null for a type without default constructor"_test = [] {
        const TypeInfo& info = registry().get<TrNoDefault>();

        expect(info.construct == nullptr);
        expect(info.copy != nullptr);
        expect(info.destroy != nullptr);
    };
}

void test_ref_make_and_as() {
    "Ref::make wraps a typed object and as<T> reads it back"_test = [] {
        TrVec2 vec{1.0f, 2.0f};
        Ref ref = Ref::make(vec);

        expect(ref.valid());
        expect(&ref.type() == &registry().get<TrVec2>());
        expect(ref.data() == &vec);
        expect(&ref.as<TrVec2>() == &vec);
    };
}

void test_ref_field_chain_reads_and_writes() {
    "Ref::field chains by name and writes to the original object"_test = [] {
        TrActor actor{"hero", 100, {{1.0f, 2.0f}, 80.0, {}}};
        Ref root = Ref::make(actor);

        root.field("body").field("position").field("x").as<float>() += 1.0f;
        root.field("health").as<std::int32_t>() -= 30;
        root.field("name").as<std::string>() = "villain";

        expect(actor.body.position.x == 2.0_f);
        expect(actor.health == 70_i);
        expect(actor.name == std::string("villain"));
    };
}

void test_ref_unknown_field_is_invalid() {
    "an unknown field gives an invalid Ref and the chain stays invalid"_test = [] {
        TrActor actor{};
        Ref root = Ref::make(actor);

        expect(!root.field("mana").valid());
        expect(!root.field("mana").field("x").valid());
        expect(!root.field("health").field("x").valid());   // scalar has no fields
        expect(!Ref{}.field("x").valid());
    };
}

void test_ref_path() {
    "Ref::path follows dotted field names like chained field() calls"_test = [] {
        TrActor actor{"hero", 100, {{1.0f, 2.0f}, 80.0, {}}};
        Ref root = Ref::make(actor);

        root.path("body.position.y").as<float>() = 5.0f;

        expect(actor.body.position.y == 5.0_f);
        expect(root.path("body.position").data() == root.field("body").field("position").data());
        expect(root.path("health").tryAs<std::int32_t>() == &actor.health);
    };
}

void test_ref_path_empty_returns_self() {
    "Ref::path with an empty path returns the Ref itself"_test = [] {
        TrVec2 vec{};
        Ref ref = Ref::make(vec);

        expect(ref.path("").data() == &vec);
        expect(&ref.path("").type() == &ref.type());
        expect(!Ref{}.path("").valid());
    };
}

void test_ref_path_invalid() {
    "Ref::path is invalid on a missing or empty segment"_test = [] {
        TrActor actor{};
        Ref root = Ref::make(actor);

        expect(!root.path("body.scale").valid());
        expect(!root.path("mana.x").valid());
        expect(!root.path("health.x").valid());
        expect(!root.path("body..position").valid());
        expect(!root.path(".body").valid());
        expect(!root.path("body.").valid());
        expect(!Ref{}.path("body").valid());
    };
}

void test_ref_type_mismatch() {
    "as<T> throws and tryAs<T> returns null on type mismatch"_test = [] {
        TrVec2 vec{1.0f, 2.0f};
        Ref x = Ref::make(vec).field("x");

        expect(x.tryAs<double>() == nullptr);
        expect(x.tryAs<float>() == &vec.x);
        expect(throws<std::logic_error>([&] { (void)x.as<double>(); }));
        expect(throws<std::logic_error>([] { (void)Ref{}.as<float>(); }));
    };
}

void test_ref_iterates_fields_generically() {
    "a Ref can be walked through TypeInfo::fields without knowing the type"_test = [] {
        TrVec2 vec{3.0f, 4.0f};
        Ref ref = Ref::make(vec);
        float sum = 0.0f;
        for (const auto& field : ref.type().fields) {
            if (float* value = ref.field(field).tryAs<float>()) {
                sum += *value;
            }
        }

        expect(sum == 7.0_f);
    };
}

}

void run_type_registry_tests() {
    test_type_id_is_a_stable_name_hash();
    test_builtin_scalars();
    test_register_is_idempotent();
    test_find_without_registering();
    test_lookup_by_name_and_id();
    test_struct_fields();
    test_field_types_are_registered_recursively();
    test_unreflected_field_type_is_opaque();
    test_find_field();
    test_field_accessor();
    test_name_collision_throws();
    test_types_lists_registrations();
    test_lifetime_operations();
    test_no_default_constructor();
    test_ref_make_and_as();
    test_ref_field_chain_reads_and_writes();
    test_ref_unknown_field_is_invalid();
    test_ref_path();
    test_ref_path_empty_returns_self();
    test_ref_path_invalid();
    test_ref_type_mismatch();
    test_ref_iterates_fields_generically();
}
