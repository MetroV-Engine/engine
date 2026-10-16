#include <boost/ut.hpp>

#include <algorithm>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <map>
#include <new>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "ECS/Reflection/Reflection.hpp"

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
        TrCounted& operator=(const TrCounted&) = default;
        ~TrCounted() { --alive; }
    };

    // Fields whose types are fundamental integers rather than <cstdint> aliases.
    struct TrSized {
        std::size_t count;
        long delta;
        std::map<std::size_t, int> byIndex;
    };

    // Claims the built-in name "uint64" (hand-written Descriptor below).
    struct TrUint64Impostor { int value; };

    // Rollback fixtures. Their Descriptors are hand-written (no E_REFLECT) so
    // that the failing registrations are not attempted before main.
    struct TrRbTaken { int a; };   // E_REFLECT: owns the name "TrRbTaken"

    namespace other {
        struct TrRbTaken { int b; };   // also named "TrRbTaken": cannot register
    }

    struct TrRbInner { float value; };

    struct TrRbBroken {
        TrRbInner inner;
        std::vector<TrRbInner> inners;
        other::TrRbTaken bad;
    };

    struct TrRbNode {
        std::vector<TrRbNode> children;
        other::TrRbTaken bad;
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

E_REFLECT(TrSized)
    E_FIELD(count, std::size_t)
    E_FIELD(delta, long)
    E_FIELD(byIndex, std::map<std::size_t, int>)
E_END

E_REFLECT(TrRbTaken)
    E_FIELD(a, int)
E_END

template<> struct ECS::reflect::Descriptor<TrUint64Impostor> {
    static constexpr std::string_view name = "uint64";
    template<typename Visitor> static void forEach(Visitor&) {}
};

template<> struct ECS::reflect::Descriptor<other::TrRbTaken> {
    static constexpr std::string_view name = "TrRbTaken";
    template<typename Visitor> static void forEach(Visitor&) {}
};

template<> struct ECS::reflect::Descriptor<TrRbInner> {
    static constexpr std::string_view name = "TrRbInner";
    template<typename Visitor> static void forEach(Visitor& v) {
        ECS::reflect::detail::visitField<&TrRbInner::value>(v, "value");
    }
};

template<> struct ECS::reflect::Descriptor<TrRbBroken> {
    static constexpr std::string_view name = "TrRbBroken";
    template<typename Visitor> static void forEach(Visitor& v) {
        ECS::reflect::detail::visitField<&TrRbBroken::inner>(v, "inner");
        ECS::reflect::detail::visitField<&TrRbBroken::inners>(v, "inners");
        ECS::reflect::detail::visitField<&TrRbBroken::bad>(v, "bad");
    }
};

template<> struct ECS::reflect::Descriptor<TrRbNode> {
    static constexpr std::string_view name = "TrRbNode";
    template<typename Visitor> static void forEach(Visitor& v) {
        ECS::reflect::detail::visitField<&TrRbNode::children>(v, "children");
        ECS::reflect::detail::visitField<&TrRbNode::bad>(v, "bad");
    }
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
        expect(!body.fields[0].readOnly);
    };
}

void test_field_types_are_registered_recursively() {
    "registering a type registers its field types"_test = [] {
        (void)registry().get<TrActor>();

        expect(registry().find("TrBody") != nullptr);
        expect(registry().find("TrVec2") != nullptr);
    };
}

void test_unreflected_type_is_opaque() {
    "a type that is neither scalar, reflected, container nor enum is Opaque"_test = [] {
        struct Unreflected { int a; double b; };
        const TypeInfo& info = registry().get<Unreflected>();

        expect(info.kind == TypeKind::Opaque);
        expect(info.fields.empty());
        expect(info.size == sizeof(Unreflected));
        expect(!info.sequenceOps && !info.mapOps && !info.optionalOps && !info.enumOps);
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

// --- Built-in scalars ----------------------------------------------------

// Writes value with the print operation of T's TypeInfo; empty if none.
template<typename T>
std::string printed(T value) {
    const TypeInfo* info = registry().find<T>();
    std::ostringstream os;
    if (info && info->print) {
        info->print(os, &value);
    }
    return os.str();
}

// Expected name of an integer type: signedness and width in bits.
template<typename T>
std::string integerName() {
    return std::string(std::is_signed_v<T> ? "int" : "uint") + std::to_string(sizeof(T) * CHAR_BIT);
}

template<typename T>
bool isIntegerScalar() {
    const TypeInfo* info = registry().find<T>();
    return info && info->kind == TypeKind::Scalar && info->name == integerName<T>() &&
           info->size == sizeof(T) && info->align == alignof(T) &&
           info->id == ECS::reflect::typeId(info->name) && registry().find(info->name) == info;
}

std::size_t timesListed(const TypeInfo* info) {
    const auto& types = registry().types();
    return static_cast<std::size_t>(std::count(types.begin(), types.end(), info));
}

void test_builtin_scalar_names_unchanged() {
    "the historical scalars keep their names, ids and registration order"_test = [] {
        const std::vector<std::pair<std::string_view, const TypeInfo*>> expected{
            {"bool", registry().find<bool>()},
            {"char", registry().find<char>()},
            {"int8", registry().find<std::int8_t>()},
            {"int16", registry().find<std::int16_t>()},
            {"int32", registry().find<std::int32_t>()},
            {"int64", registry().find<std::int64_t>()},
            {"uint8", registry().find<std::uint8_t>()},
            {"uint16", registry().find<std::uint16_t>()},
            {"uint32", registry().find<std::uint32_t>()},
            {"uint64", registry().find<std::uint64_t>()},
            {"float", registry().find<float>()},
            {"double", registry().find<double>()},
            {"string", registry().find<std::string>()},
        };
        const auto& types = registry().types();
        expect((types.size() >= expected.size()) >> fatal);

        for (std::size_t i = 0; i < expected.size(); ++i) {
            const auto& [name, info] = expected[i];
            expect((info != nullptr) >> fatal);
            expect(info->name == name);
            expect(info->kind == TypeKind::Scalar);
            expect(info->id == ECS::reflect::typeId(name));
            expect(types[i] == info);
        }
    };
}

void test_every_fundamental_integer_is_a_scalar() {
    "every fundamental and <cstdint> integer type is a Scalar named by signedness and width"_test = [] {
        expect(isIntegerScalar<signed char>());
        expect(isIntegerScalar<short>());
        expect(isIntegerScalar<int>());
        expect(isIntegerScalar<long>());
        expect(isIntegerScalar<long long>());
        expect(isIntegerScalar<unsigned char>());
        expect(isIntegerScalar<unsigned short>());
        expect(isIntegerScalar<unsigned int>());
        expect(isIntegerScalar<unsigned long>());
        expect(isIntegerScalar<unsigned long long>());
        expect(isIntegerScalar<std::int8_t>());
        expect(isIntegerScalar<std::int64_t>());
        expect(isIntegerScalar<std::uint64_t>());
        expect(isIntegerScalar<std::size_t>());
        expect(isIntegerScalar<std::ptrdiff_t>());
        expect(isIntegerScalar<const unsigned long>());
    };
}

void test_integer_aliases_share_type_info() {
    "integer types of the same width share one TypeInfo, listed once"_test = [] {
        if constexpr (sizeof(unsigned long) == sizeof(unsigned long long)) {
            expect(registry().find<unsigned long>() == registry().find<unsigned long long>());
        }
        if constexpr (sizeof(long) == sizeof(long long)) {
            expect(registry().find<long>() == registry().find<long long>());
        }
        if constexpr (sizeof(unsigned long long) == sizeof(std::uint64_t)) {
            expect(registry().find<unsigned long long>() == registry().find<std::uint64_t>());
            expect(registry().find<long long>() == registry().find<std::int64_t>());
        }
        if constexpr (sizeof(int) == sizeof(std::int32_t)) {
            expect(registry().find<int>() == registry().find<std::int32_t>());
            expect(registry().find<unsigned int>() == registry().find<std::uint32_t>());
        }
        expect(registry().find<signed char>() == registry().find<std::int8_t>());
        expect(registry().find<unsigned char>() == registry().find<std::uint8_t>());
        expect(registry().find<short>() == registry().find<std::int16_t>());

        // Aliases add lookup keys, not entries.
        for (const TypeInfo* info : {registry().find<long>(), registry().find<long long>(),
                                     registry().find<unsigned long>(),
                                     registry().find<unsigned long long>(),
                                     registry().find<int>(), registry().find<signed char>()}) {
            expect(timesListed(info) == 1_ul);
        }
    };
}

void test_char_types_are_distinct() {
    "char, signed char and unsigned char stay three distinct types"_test = [] {
        expect(registry().find<char>()->name == std::string("char"));
        expect(registry().find<char>() != registry().find<signed char>());
        expect(registry().find<char>() != registry().find<unsigned char>());
        expect(printed('A') == "A");   // char still prints as a character
    };
}

void test_builtin_name_cannot_be_claimed() {
    "a reflected type named like a built-in integer is rejected"_test = [] {
        const TypeInfo* uint64 = registry().find<std::uint64_t>();

        expect(throws<std::logic_error>([] { (void)registry().get<TrUint64Impostor>(); }));
        expect(registry().find<TrUint64Impostor>() == nullptr);
        expect(registry().find("uint64") == uint64);
        expect(registry().find<std::uint64_t>() == uint64);
    };
}

void test_int8_prints_as_number() {
    "int8 and uint8 (signed / unsigned char) print as numbers, not characters"_test = [] {
        expect(printed<std::int8_t>(-5) == "-5");
        expect(printed<std::uint8_t>(200) == "200");
        expect(printed<signed char>(65) == "65");
        expect(printed<unsigned char>(65) == "65");
        expect(printed(true) == "true");
    };
}

void test_character_and_long_double_scalars() {
    "wchar_t, char8_t, char16_t, char32_t and long double are printable Scalars"_test = [] {
        const std::vector<std::pair<std::string_view, const TypeInfo*>> expected{
            {"wchar", registry().find<wchar_t>()},
            {"char8", registry().find<char8_t>()},
            {"char16", registry().find<char16_t>()},
            {"char32", registry().find<char32_t>()},
            {"long double", registry().find<long double>()},
        };
        for (const auto& [name, info] : expected) {
            expect((info != nullptr) >> fatal);
            expect(info->name == name);
            expect(info->kind == TypeKind::Scalar);
            expect(info->print != nullptr);
            expect(registry().find(name) == info);
            expect(timesListed(info) == 1_ul);
        }
        expect(registry().find<long double>() != registry().find<double>());

        // Character types print their numeric code unit.
        expect(printed(L'A') == "65");
        expect(printed(u8'A') == "65");
        expect(printed(char16_t{0x00E9}) == "233");
        expect(printed(char32_t{0x1F600}) == "128512");
        expect(printed(char8_t{0xFF}) == "255");
        expect(printed(1.5L) == "1.5");
    };
}

void test_size_t_field_is_fully_usable() {
    "std::size_t and long fields are editable, printable and usable as map keys"_test = [] {
        TrSized sized{40, -3, {{7, 70}}};
        const TypeInfo& info = registry().get<TrSized>();
        const TypeInfo* count = info.findField("count")->type;
        const TypeInfo* delta = info.findField("delta")->type;

        expect(count->kind == TypeKind::Scalar);
        expect(count->name == integerName<std::size_t>());
        expect(count == registry().find<std::size_t>());
        expect(delta->kind == TypeKind::Scalar);
        expect(delta->name == integerName<long>());

        Ref root = Ref::make(sized);
        root.field("count").as<std::size_t>() += 2;
        root.path("delta").as<long>() = 5;
        expect(sized.count == 42_ul);
        expect(sized.delta == 5_l);
        expect(printed(sized.count) == "42");
        expect(printed(sized.delta) == "5");

        expect(root.path("byIndex[7]").as<int>() == 70_i);
        std::size_t key = 9;
        root.field("byIndex").insert(Ref::make(key)).as<int>() = 90;
        expect(sized.byIndex.at(9) == 90_i);
        expect(root.path("byIndex[9]").as<int>() == 90_i);
    };
}

void test_size_t_map_key_parse_failures() {
    "a std::size_t map key rejects missing, negative and malformed keys"_test = [] {
        TrSized sized{0, 0, {{7, 70}}};
        Ref root = Ref::make(sized);

        expect(!root.path("byIndex[8]").valid());
        expect(!root.path("byIndex[-7]").valid());
        expect(!root.path("byIndex[7x]").valid());
        expect(!root.path("byIndex[]").valid());
    };
}

// --- Rollback of a failed registration -----------------------------------

void test_failed_registration_is_rolled_back() {
    "a struct whose field fails to register is removed with its field types"_test = [] {
        const std::size_t before = registry().types().size();
        const TypeInfo* taken = registry().find<TrRbTaken>();

        expect(throws<std::logic_error>([] { (void)registry().get<TrRbBroken>(); }));
        expect(registry().find<TrRbBroken>() == nullptr);
        expect(registry().find("TrRbBroken") == nullptr);
        expect(registry().find(ECS::reflect::typeId("TrRbBroken")) == nullptr);
        // Field types registered on its behalf are gone too.
        expect(registry().find<TrRbInner>() == nullptr);
        expect(registry().find("TrRbInner") == nullptr);
        expect(registry().find<std::vector<TrRbInner>>() == nullptr);
        expect(registry().find<other::TrRbTaken>() == nullptr);
        // What existed before is untouched.
        expect(taken != nullptr && registry().find("TrRbTaken") == taken);
        expect(registry().types().size() == before);
    };
}

void test_failed_self_referencing_registration_is_rolled_back() {
    "a self-referencing struct is rolled back with the container pointing to it"_test = [] {
        const std::size_t before = registry().types().size();

        expect(throws<std::logic_error>([] { (void)registry().get<TrRbNode>(); }));
        expect(registry().find<TrRbNode>() == nullptr);
        expect(registry().find("TrRbNode") == nullptr);
        expect(registry().find<std::vector<TrRbNode>>() == nullptr);
        expect(registry().types().size() == before);
    };
}

void test_registration_after_rollback() {
    "after a rollback, other types register and a retry fails the same way"_test = [] {
        const std::size_t before = registry().types().size();

        const TypeInfo& inner = registry().get<TrRbInner>();
        expect(inner.kind == TypeKind::Struct);
        expect(inner.fields.size() == 1_ul);
        expect(registry().find("TrRbInner") == &inner);
        expect((registry().types().size() == before + 1) >> fatal);
        expect(registry().types().back() == &inner);

        // Retrying still collides; types registered before the call are kept.
        expect(throws<std::logic_error>([] { (void)registry().get<TrRbBroken>(); }));
        expect(throws<std::logic_error>([] { (void)registry().get<TrRbNode>(); }));
        expect(registry().find<TrRbBroken>() == nullptr);
        expect(registry().find<TrRbNode>() == nullptr);
        expect(registry().find<std::vector<TrRbInner>>() == nullptr);
        expect(registry().find<std::vector<TrRbNode>>() == nullptr);
        expect(registry().find<TrRbInner>() == &inner);
        expect(registry().types().size() == before + 1);
        expect(timesListed(&inner) == 1_ul);
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
    test_unreflected_type_is_opaque();
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
    test_builtin_scalar_names_unchanged();
    test_every_fundamental_integer_is_a_scalar();
    test_integer_aliases_share_type_info();
    test_char_types_are_distinct();
    test_builtin_name_cannot_be_claimed();
    test_int8_prints_as_number();
    test_character_and_long_double_scalars();
    test_size_t_field_is_fully_usable();
    test_size_t_map_key_parse_failures();
    test_failed_registration_is_rolled_back();
    test_failed_self_referencing_registration_is_rolled_back();
    test_registration_after_rollback();
}
