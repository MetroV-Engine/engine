#include <boost/ut.hpp>

#include <set>
#include <string>
#include <unordered_set>

#include "ECS/ECS.hpp"
#include "Engine/Components/Uuid.hpp"
#include "ECS/Reflection/Reflection.hpp"

using namespace boost::ut;

void run_uuid_tests() {
    // ---- isNull

    "a default Uuid is null"_test = [] {
        Uuid id;
        expect(uuid::isNull(id));
        expect(id.high == 0u);
        expect(id.low == 0u);
    };

    "a Uuid with only one half set is not null"_test = [] {
        expect(!uuid::isNull(Uuid{1, 0}));
        expect(!uuid::isNull(Uuid{0, 1}));
    };

    // ---- generate

    "generate returns a non-null Uuid"_test = [] {
        for (int i = 0; i < 100; ++i)
            expect(!uuid::isNull(uuid::generate()));
    };

    "generate returns a different Uuid every time"_test = [] {
        std::set<Uuid> seen;
        for (int i = 0; i < 2000; ++i)
            seen.insert(uuid::generate());
        expect(seen.size() == std::size_t{2000});
    };

    "generate sets the version 4 and variant bits"_test = [] {
        for (int i = 0; i < 50; ++i) {
            const std::string text = uuid::toString(uuid::generate());
            expect(text[14] == '4');
            expect(std::string("89ab").find(text[19]) != std::string::npos);
        }
    };

    // ---- toString

    "toString of a null Uuid is all zeros"_test = [] {
        expect(uuid::toString(Uuid{}) == std::string("00000000-0000-0000-0000-000000000000"));
    };

    "toString writes the 8-4-4-4-12 form in lower case"_test = [] {
        const Uuid id{0x0123456789ABCDEFull, 0xFEDCBA9876543210ull};
        expect(uuid::toString(id) == std::string("01234567-89ab-cdef-fedc-ba9876543210"));
    };

    "toString is always 36 characters"_test = [] {
        expect(uuid::toString(Uuid{1, 1}).size() == std::size_t{36});
        expect(uuid::toString(uuid::generate()).size() == std::size_t{36});
    };

    // ---- fromString

    "fromString reads what toString wrote"_test = [] {
        for (int i = 0; i < 100; ++i) {
            const Uuid id = uuid::generate();
            const auto parsed = uuid::fromString(uuid::toString(id));
            expect(parsed.has_value());
            expect(*parsed == id);
        }
    };

    "fromString accepts upper case"_test = [] {
        const auto parsed = uuid::fromString("01234567-89AB-CDEF-FEDC-BA9876543210");
        expect(parsed.has_value());
        expect(parsed->high == 0x0123456789ABCDEFull);
        expect(parsed->low == 0xFEDCBA9876543210ull);
    };

    "fromString reads the null Uuid"_test = [] {
        const auto parsed = uuid::fromString("00000000-0000-0000-0000-000000000000");
        expect(parsed.has_value());
        expect(uuid::isNull(*parsed));
    };

    "fromString rejects an empty text"_test = [] {
        expect(!uuid::fromString("").has_value());
    };

    "fromString rejects a wrong length"_test = [] {
        expect(!uuid::fromString("01234567-89ab-cdef-fedc-ba987654321").has_value());
        expect(!uuid::fromString("01234567-89ab-cdef-fedc-ba98765432100").has_value());
    };

    "fromString rejects misplaced or missing dashes"_test = [] {
        expect(!uuid::fromString("0123456789ab-cdef-fedc-ba9876543210-").has_value());
        expect(!uuid::fromString("01234567_89ab-cdef-fedc-ba9876543210").has_value());
        expect(!uuid::fromString("012345678-9ab-cdef-fedc-ba9876543210").has_value());
    };

    "fromString rejects a non hexadecimal digit"_test = [] {
        expect(!uuid::fromString("0123456g-89ab-cdef-fedc-ba9876543210").has_value());
        expect(!uuid::fromString("01234567-89ab-cdef-fedc-ba987654321 ").has_value());
    };

    // ---- comparison and hash

    "equal Uuids compare equal and have the same hash"_test = [] {
        const Uuid a{3, 4};
        const Uuid b{3, 4};
        expect(a == b);
        expect(std::hash<Uuid>{}(a) == std::hash<Uuid>{}(b));
    };

    "different Uuids compare different"_test = [] {
        expect(Uuid{3, 4} != Uuid{3, 5});
        expect(Uuid{3, 4} != Uuid{2, 4});
    };

    "Uuids are ordered by high then low"_test = [] {
        expect(Uuid{1, 9} < Uuid{2, 0});
        expect(Uuid{2, 0} < Uuid{2, 1});
    };

    "a Uuid can be a key of an unordered set"_test = [] {
        std::unordered_set<Uuid> set;
        const Uuid a = uuid::generate();
        set.insert(a);
        set.insert(a);
        expect(set.size() == std::size_t{1});
        expect(set.count(a) == std::size_t{1});
    };

    // ---- as a component

    "a Uuid is a component of an entity"_test = [] {
        ECS::Registry world;
        const ECS::Entity entity = world.spawnEntity();
        const Uuid id = uuid::generate();
        world.addComponent(entity, Uuid{id});
        expect(world.getComponent<Uuid>(entity) == id);
    };

    "a Uuid is reflected and listed among the component types"_test = [] {
        const ECS::reflect::TypeInfo* type = ECS::reflect::RegistryInspector::findType("Uuid");
        expect(type != nullptr);
        expect(type->findField("high") != nullptr);
        expect(type->findField("low") != nullptr);
    };
}
