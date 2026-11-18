#include <boost/ut.hpp>

#include <memory>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

#include "ECS/Reflection/PrettyPrint.hpp"
#include "ECS/Reflection/Reflection.hpp"

using namespace boost::ut;

// The registry is a process-wide singleton: names are prefixed with "Rv" so
// they never collide with another test file's E_REFLECT.
namespace {
    struct RvBox { float halfX = 0.5f; float halfY = 0.5f; };
    struct RvCircle { float radius = 0.5f; };
    using RvShape = std::variant<RvBox, RvCircle>;

    struct RvBody {
        RvShape shape;
        std::variant<int, std::string> tag;
    };

    struct RvNoDefault {
        explicit RvNoDefault(int v) : value(v) {}
        int value;
    };

    // Default construction throws on demand: emplacing it leaves its variant
    // valueless. Not trivially copyable (the vector): for such types the
    // standard libraries build the new value in place, after destroying the
    // old one; a trivially copyable one is built aside first and never
    // leaves the variant valueless.
    struct RvFragile {
        static inline bool fail = false;
        std::vector<int> data;
        RvFragile() {
            if (fail) {
                throw std::runtime_error("RvFragile");
            }
        }
    };
}

E_REFLECT(RvBox)
    E_FIELD(halfX, float)
    E_FIELD(halfY, float)
E_END

E_REFLECT(RvCircle)
    E_FIELD(radius, float)
E_END

E_REFLECT(RvBody)
    E_FIELD(shape, RvShape)
    E_FIELD(tag, std::variant<int, std::string>)
E_END

using ECS::reflect::Ref;
using ECS::reflect::TypeInfo;
using ECS::reflect::TypeKind;
using ECS::reflect::TypeRegistry;

namespace {
    const TypeInfo& infoOf(const auto& value) {
        return TypeRegistry::instance().registerType<std::remove_cvref_t<decltype(value)>>();
    }
}

void run_variant_tests() {
    "a std::variant is a Variant named after its alternatives, in order"_test = [] {
        const TypeInfo& type = TypeRegistry::instance().registerType<std::variant<int, std::string>>();

        expect(type.kind == TypeKind::Variant);
        expect(type.name == "variant<int32,string>");
        expect((type.variantOps.has_value()) >> fatal);
        expect(type.variantOps->alternatives.size() == 2_u);
        expect(type.variantOps->alternatives[0] == TypeRegistry::instance().find<int>());
        expect(type.variantOps->alternatives[1] == TypeRegistry::instance().find<std::string>());
        expect(type.construct != nullptr && type.copy != nullptr);
        expect(TypeRegistry::instance().find("variant<int32,string>") == &type);
    };

    "registering alternatives registers reflected structs too"_test = [] {
        const TypeInfo& shape = TypeRegistry::instance().registerType<RvShape>();

        expect(shape.name == "variant<RvBox,RvCircle>");
        expect(shape.variantOps->alternatives[0]->kind == TypeKind::Struct);
        expect(shape.variantOps->alternatives[1]->name == "RvCircle");
    };

    "the held alternative: its index and a Ref to it"_test = [] {
        RvShape shape = RvCircle{2.f};
        const Ref ref = Ref::make(shape);

        expect(ref.alternativeIndex() == std::optional<std::size_t>(1));
        const Ref held = ref.alternative();
        expect((held.valid()) >> fatal);
        expect(held.data() == &std::get<RvCircle>(shape));
        expect(held.field("radius").as<float>() == 2._f);

        held.field("radius").as<float>() = 3.f;
        expect(std::get<RvCircle>(shape).radius == 3._f) << "written in place";
    };

    "emplaceAlternative switches to a default-constructed alternative"_test = [] {
        RvShape shape = RvCircle{2.f};
        const Ref ref = Ref::make(shape);

        const Ref box = ref.emplaceAlternative(0);

        expect((box.valid()) >> fatal);
        expect(std::holds_alternative<RvBox>(shape));
        expect(box.data() == &std::get<RvBox>(shape));
        expect(std::get<RvBox>(shape).halfX == 0.5_f) << "default-constructed";

        std::get<RvBox>(shape).halfX = 4.f;
        ref.emplaceAlternative(0);
        expect(std::get<RvBox>(shape).halfX == 0.5_f) << "the same alternative is replaced too";
    };

    "emplaceAlternative refuses, leaving the variant unchanged"_test = [] {
        RvShape shape = RvCircle{2.f};
        const Ref ref = Ref::make(shape);
        expect(!ref.emplaceAlternative(2).valid()) << "out of range";
        expect(std::get<RvCircle>(shape).radius == 2._f);

        std::variant<int, RvNoDefault> mixed = 7;
        const Ref mixedRef = Ref::make(mixed);
        expect(!mixedRef.emplaceAlternative(1).valid()) << "no default constructor";
        expect(std::get<int>(mixed) == 7_i);

        int notAVariant = 0;
        const Ref plain = Ref::make(notAVariant);
        expect(!plain.emplaceAlternative(0).valid());
        expect(!plain.alternativeIndex().has_value());
        expect(!plain.alternative().valid());
    };

    "a type repeated in a variant is told apart by its index"_test = [] {
        std::variant<int, int> twice{std::in_place_index<1>, 5};
        const Ref ref = Ref::make(twice);

        expect(ref.alternativeIndex() == std::optional<std::size_t>(1));
        expect(ref.alternative().as<int>() == 5_i);
        ref.emplaceAlternative(0).as<int>() = 9;
        expect(twice.index() == 0_u);
        expect(std::get<0>(twice) == 9_i);
    };

    "a path goes through the held alternative"_test = [] {
        RvBody body{RvCircle{1.5f}, std::string("hero")};
        const Ref ref = Ref::make(body);

        expect(ref.path("shape.radius").as<float>() == 1.5_f);
        expect(!ref.path("shape.halfX").valid()) << "a field the circle does not have";
        expect(ref.path("tag").alternative().as<std::string>() == "hero");
        expect(ref.path("shape").is(TypeKind::Variant)) << "the variant itself, to switch it";

        ref.path("shape").emplaceAlternative(0);
        ref.path("shape.halfY").as<float>() = 2.f;
        expect(std::get<RvBox>(body.shape).halfY == 2._f);
    };

    "a valueless variant holds nothing"_test = [] {
        std::variant<int, RvFragile> fragile = 3;
        const Ref ref = Ref::make(fragile);

        RvFragile::fail = true;
        expect(throws<std::runtime_error>([&] { (void)ref.emplaceAlternative(1); }));
        RvFragile::fail = false;

        expect((fragile.valueless_by_exception()) >> fatal);
        expect(!ref.alternativeIndex().has_value());
        expect(!ref.alternative().valid());
        expect(ECS::reflect::toPrettyString(ref) == "valueless");
        expect(ref.emplaceAlternative(0).valid()) << "emplacing repairs it";
        expect(fragile.index() == 0_u);
    };

    "a variant is copyable only if every alternative is, deeply"_test = [] {
        expect(infoOf(RvShape{}).copy != nullptr);
        // std::variant claims to be copyable here, but copying the vector
        // of unique_ptr would not compile.
        const TypeInfo& moveOnly =
            TypeRegistry::instance().registerType<std::variant<int, std::vector<std::unique_ptr<int>>>>();
        expect(moveOnly.copy == nullptr);
    };

    "the TypeInfo's lifetime operations work on a variant"_test = [] {
        const TypeInfo& type = infoOf(RvShape{});
        alignas(RvShape) unsigned char a[sizeof(RvShape)];
        alignas(RvShape) unsigned char b[sizeof(RvShape)];

        type.construct(a);
        expect(std::holds_alternative<RvBox>(*reinterpret_cast<RvShape*>(a))) << "the first alternative";
        *reinterpret_cast<RvShape*>(a) = RvCircle{6.f};
        type.copy(b, a);
        expect(std::get<RvCircle>(*reinterpret_cast<RvShape*>(b)).radius == 6._f);
        type.destroy(a);
        type.destroy(b);
    };

    "prettyPrint shows the held alternative"_test = [] {
        RvBody body{RvCircle{1.5f}, 4};
        const ECS::reflect::PrettyPrintOptions oneLine{.indent = 0};

        expect(ECS::reflect::toPrettyString(Ref::make(body), oneLine) ==
               "RvBody { shape: RvCircle { radius: 1.5 }, tag: 4 }");
    };
}
