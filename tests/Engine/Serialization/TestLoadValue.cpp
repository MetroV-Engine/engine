#include <boost/ut.hpp>

#include <cstdint>
#include <limits>
#include <string>

#include "Engine/Serialization/Serialization.hpp"
#include "Serialization/Support/Helpers.hpp"
#include "Serialization/Support/TestTypes.hpp"

using namespace boost::ut;
using ECS::reflect::Ref;
using serial::load;
using serial::Report;
using serial::serialize;
using namespace sertest;

void run_load_value_tests() {
    // ---- round trips

    "a struct reads back what was written"_test = [] {
        Actor source;
        source.mood = Mood::Angry;
        source.speed = 2.25;
        source.scale = 0.1f;
        source.alive = false;
        source.level = 200;
        source.big = std::numeric_limits<std::int64_t>::min();
        source.title = "caf\xC3\xA9 \"q\"\n\t\\";

        Actor copy;
        Report report;
        expect(load(Ref::make(copy), serialize(Ref::make(source)), &report));
        expect(report.ok());
        expect(report.warnings.empty());
        expect(copy.mood == Mood::Angry);
        expect(copy.speed == 2.25);
        expect(copy.scale == 0.1f);
        expect(!copy.alive);
        expect(copy.level == 200);
        expect(copy.big == std::numeric_limits<std::int64_t>::min());
        expect(copy.title == source.title);
    };

    "floats keep their exact value through a round trip"_test = [] {
        for (float value : {0.1f, 1.0f / 3.0f, 3.4028235e38f, 1e-30f, -0.0f}) {
            Actor source;
            source.scale = value;
            Actor copy;
            load(Ref::make(copy), serialize(Ref::make(source)));
            expect(copy.scale == value);
        }
    };

    "doubles keep their exact value through a round trip"_test = [] {
        for (double value : {0.1, 1.0 / 3.0, 1.7976931348623157e308, 5e-324}) {
            Actor source;
            source.speed = value;
            Actor copy;
            load(Ref::make(copy), serialize(Ref::make(source)));
            expect(copy.speed == value);
        }
    };

    "containers read back what was written"_test = [] {
        Inventory source;
        source.items = {{"sword", 1}, {"shield", 2}, {"potion", 5}};
        source.equipped = Item{"bow", 3};
        source.counts = {{"gold", 5}, {"arrow", 20}};
        source.loose = {{"x", 1}, {"y", 2}};
        source.byNumber = {{2, "two"}, {-10, "minus ten"}};
        source.slots = {7, 8, 9};

        Inventory copy;
        Report report;
        expect(load(Ref::make(copy), serialize(Ref::make(source)), &report));
        expect(report.warnings.empty());
        expect(copy.items.size() == std::size_t{3});
        expect(copy.items[2].id == "potion");
        expect(copy.items[2].count == 5);
        expect(copy.equipped.has_value());
        expect(copy.equipped->id == "bow");
        expect(same(copy.counts, source.counts));
        expect(same(copy.loose, source.loose));
        expect(same(copy.byNumber, source.byNumber));
        expect(same(copy.slots, source.slots));
    };

    "a Uuid field reads back"_test = [] {
        Owner source;
        source.asset = uuid::generate();
        Owner copy;
        expect(load(Ref::make(copy), serialize(Ref::make(source))));
        expect(copy.asset == source.asset);
    };

    "a scalar can be loaded alone"_test = [] {
        int number = 0;
        expect(load(Ref::make(number), "42"));
        expect(number == 42);
    };

    // ---- merging into an existing value

    "a missing field keeps its current value, without a warning"_test = [] {
        Health health{1, 2};
        Report report;
        expect(load(Ref::make(health), R"({"current":5})", &report));
        expect(health.current == 5);
        expect(health.max == 2);
        expect(report.warnings.empty());
    };

    "an empty object changes nothing"_test = [] {
        Health health{1, 2};
        expect(load(Ref::make(health), "{}"));
        expect(health.current == 1);
        expect(health.max == 2);
    };

    "a vector is resized to the size of the text"_test = [] {
        Inventory inventory;
        inventory.items = {{"a", 1}, {"b", 1}, {"c", 1}};
        expect(load(Ref::make(inventory), R"({"items":[{"id":"z","count":9}]})"));
        expect(inventory.items.size() == std::size_t{1});
        expect(inventory.items[0].id == "z");
    };

    "a map takes the content of the text, like a vector"_test = [] {
        Inventory inventory;
        inventory.counts = {{"gone", 1}, {"over", 1}};
        Report report;
        expect(load(Ref::make(inventory), R"({"counts":{"over":9,"new":2}})", &report));
        expect(report.warnings.empty());
        expect(inventory.counts.size() == std::size_t{2});
        expect(inventory.counts.count("gone") == std::size_t{0});
        expect(inventory.counts["over"] == 9);
        expect(inventory.counts["new"] == 2);
    };

    "an unordered map and a map with integer keys are replaced too"_test = [] {
        Inventory inventory;
        inventory.loose = {{"a", 1}, {"b", 2}};
        inventory.byNumber = {{1, "one"}, {2, "two"}};
        expect(load(Ref::make(inventory), R"({"loose":{"c":3},"byNumber":{"7":"seven"}})"));
        expect(inventory.loose.size() == std::size_t{1});
        expect(inventory.loose["c"] == 3);
        expect(inventory.byNumber.size() == std::size_t{1});
        expect(inventory.byNumber[7] == "seven");
    };

    "an empty object empties a map"_test = [] {
        Inventory inventory;
        inventory.counts = {{"a", 1}, {"b", 2}, {"c", 3}};
        expect(load(Ref::make(inventory), R"({"counts":{}})"));
        expect(inventory.counts.empty());
    };

    "a map absent from the text keeps its entries"_test = [] {
        Inventory inventory;
        inventory.counts = {{"keep", 1}};
        expect(load(Ref::make(inventory), R"({"slots":[1,2,3]})"));
        expect(inventory.counts.size() == std::size_t{1});
    };

    "a map is left alone when the text is not an object"_test = [] {
        Inventory inventory;
        inventory.counts = {{"keep", 1}};
        Report report;
        expect(load(Ref::make(inventory), R"({"counts":[1,2]})", &report));
        expect(inventory.counts.size() == std::size_t{1});
        expect(hasWarning(report, "Inventory.counts"));
    };

    "a map entry that cannot be read is dropped along with the old entries"_test = [] {
        Inventory inventory;
        inventory.byNumber = {{1, "old"}};
        Report report;
        expect(load(Ref::make(inventory), R"({"byNumber":{"x":"bad","5":"five"}})", &report));
        expect(inventory.byNumber.size() == std::size_t{1});
        expect(inventory.byNumber[5] == "five");
        expect(hasWarning(report, "byNumber[x]"));
    };

    "null empties an optional and a value fills it"_test = [] {
        Inventory inventory;
        inventory.equipped = Item{"old", 1};
        expect(load(Ref::make(inventory), R"({"equipped":null})"));
        expect(!inventory.equipped.has_value());
        expect(load(Ref::make(inventory), R"({"equipped":{"id":"new"}})"));
        expect(inventory.equipped.has_value());
        expect(inventory.equipped->id == "new");
        expect(inventory.equipped->count == 1);
    };

    "an integer is accepted for a floating point field"_test = [] {
        Actor actor;
        Report report;
        expect(load(Ref::make(actor), R"({"speed":3,"scale":2})", &report));
        expect(actor.speed == 3.0);
        expect(actor.scale == 2.0f);
        expect(report.warnings.empty());
    };

    // ---- tolerance: a warning, the value is kept

    "an unknown field is a warning"_test = [] {
        Health health;
        Report report;
        expect(load(Ref::make(health), R"({"current":5,"mana":9})", &report));
        expect(health.current == 5);
        expect(report.ok());
        expect(hasWarning(report, "Health.mana"));
    };

    "a value of the wrong type is a warning and keeps the current value"_test = [] {
        Health health{1, 2};
        Report report;
        expect(load(Ref::make(health), R"({"current":"high","max":7})", &report));
        expect(health.current == 1);
        expect(health.max == 7);
        expect(hasWarning(report, "Health.current"));
    };

    "an integer out of range for the field is a warning"_test = [] {
        Actor actor;
        Report report;
        expect(load(Ref::make(actor), R"({"level":300})", &report));
        expect(actor.level == 1);
        expect(hasWarning(report, "Actor.level"));
    };

    "a negative integer for an unsigned field is a warning"_test = [] {
        Actor actor;
        Report report;
        expect(load(Ref::make(actor), R"({"level":-1})", &report));
        expect(actor.level == 1);
        expect(hasWarning(report, "Actor.level"));
    };

    "a fractional number for an integer field is a warning"_test = [] {
        Health health;
        Report report;
        expect(load(Ref::make(health), R"({"current":1.5})", &report));
        expect(health.current == 100);
        expect(hasWarning(report, "Health.current"));
    };

    "a number for a boolean or a string field is a warning"_test = [] {
        Actor actor;
        Report report;
        expect(load(Ref::make(actor), R"({"alive":1,"title":3})", &report));
        expect(actor.alive);
        expect(actor.title.empty());
        expect(report.warnings.size() == std::size_t{2});
    };

    "a non-integer for an enum is a warning"_test = [] {
        Actor actor;
        Report report;
        expect(load(Ref::make(actor), R"({"mood":"angry"})", &report));
        expect(actor.mood == Mood::Calm);
        expect(hasWarning(report, "Actor.mood"));
    };

    "an enum value is kept as is, even if no name exists for it"_test = [] {
        Actor actor;
        expect(load(Ref::make(actor), R"({"mood":42})"));
        expect(static_cast<int>(actor.mood) == 42);
    };

    "an object where an array is expected is a warning"_test = [] {
        Inventory inventory;
        Report report;
        expect(load(Ref::make(inventory), R"({"items":{"a":1}})", &report));
        expect(inventory.items.empty());
        expect(hasWarning(report, "Inventory.items"));
    };

    "a fixed array of the wrong size fills what fits, with a warning"_test = [] {
        Inventory inventory;
        Report report;
        expect(load(Ref::make(inventory), R"({"slots":[1,2]})", &report));
        expect(inventory.slots[0] == 1);
        expect(inventory.slots[1] == 2);
        expect(inventory.slots[2] == 0);
        expect(hasWarning(report, "Inventory.slots"));

        Report tooLong;
        expect(load(Ref::make(inventory), R"({"slots":[1,2,3,4]})", &tooLong));
        expect(inventory.slots[2] == 3);
        expect(hasWarning(tooLong, "Inventory.slots"));
    };

    "a map key that cannot be read is skipped with a warning"_test = [] {
        Inventory inventory;
        Report report;
        expect(load(Ref::make(inventory), R"({"byNumber":{"abc":"x","4":"four"}})", &report));
        expect(inventory.byNumber.size() == std::size_t{1});
        expect(inventory.byNumber[4] == "four");
        expect(hasWarning(report, "byNumber[abc]"));
    };

    "an opaque field in the text is skipped with a warning"_test = [] {
        WithBlob value;
        Report report;
        expect(load(Ref::make(value), R"({"kept":5,"blob":{"a":1}})", &report));
        expect(value.kept == 5);
        expect(value.blob.a == 0);
        expect(hasWarning(report, "WithBlob.blob"));
    };

    "a bad Uuid text is a warning"_test = [] {
        Owner owner;
        Report report;
        expect(load(Ref::make(owner), R"({"asset":"nope"})", &report));
        expect(uuid::isNull(owner.asset));
        expect(hasWarning(report, "Owner.asset"));
    };

    "an entity reference cannot be resolved without a world"_test = [] {
        Link link;
        Report report;
        expect(load(Ref::make(link), R"({"target":"01234567-89ab-cdef-fedc-ba9876543210"})", &report));
        expect(link.target.world() == ECS::InvalidWorld);
        expect(hasWarning(report, "Link.target"));
    };

    "a null entity reference sets no entity"_test = [] {
        ECS::Registry world;
        Link link;
        link.target = world.spawnEntity();
        Report report;
        expect(load(Ref::make(link), R"({"target":null})", &report));
        expect(link.target.world() == ECS::InvalidWorld);
        expect(report.warnings.empty());
    };

    // ---- errors

    "text that is not JSON is an error"_test = [] {
        Health health{1, 2};
        Report report;
        expect(!load(Ref::make(health), "{not json", &report));
        expect(!report.ok());
        expect(health.current == 1);
    };

    "an empty text is an error"_test = [] {
        Health health;
        Report report;
        expect(!load(Ref::make(health), "", &report));
        expect(!report.ok());
    };

    "loading into an invalid Ref is an error"_test = [] {
        Report report;
        expect(!load(Ref{}, "{}", &report));
        expect(!report.ok());
    };

    "load works without a report"_test = [] {
        Health health;
        expect(load(Ref::make(health), "{}"));
        expect(!load(Ref::make(health), "{"));
    };
}

// ---- values that used to be lost without a warning

void run_load_value_range_tests() {
    // enums
    "an enum value that fits its type is read"_test = [] {
        Sized sized;
        Report report;
        expect(load(Ref::make(sized), R"({"small":200})", &report));
        expect(sized.small == Small::High);
        expect(report.warnings.empty());
    };

    "an enum value too large for its type is a warning and keeps the current value"_test = [] {
        Sized sized;
        sized.small = Small::High;
        Report report;
        expect(load(Ref::make(sized), R"({"small":300})", &report));
        expect(sized.small == Small::High);   // not 44, what 300 would wrap to
        expect(hasWarning(report, "Sized.small"));
    };

    "a negative value for an unsigned enum is a warning and keeps the current value"_test = [] {
        Sized sized;
        Report report;
        expect(load(Ref::make(sized), R"({"small":-1})", &report));
        expect(sized.small == Small::Low);
        expect(hasWarning(report, "Sized.small"));
    };

    "the bounds of an enum's type are accepted"_test = [] {
        Sized sized;
        Report report;
        expect(load(Ref::make(sized), R"({"small":255})", &report));
        expect(static_cast<int>(sized.small) == 255);
        expect(load(Ref::make(sized), R"({"small":0})", &report));
        expect(static_cast<int>(sized.small) == 0);
        expect(report.warnings.empty());
    };

    "a negative value is accepted for a signed enum"_test = [] {
        Actor actor;
        Report report;
        expect(load(Ref::make(actor), R"({"mood":-5})", &report));
        expect(static_cast<int>(actor.mood) == -5);
        expect(report.warnings.empty());
    };

    "an enum value above the 64-bit range is a warning"_test = [] {
        Actor actor;
        Report report;
        expect(load(Ref::make(actor), R"({"mood":18446744073709551615})", &report));
        expect(actor.mood == Mood::Calm);
        expect(hasWarning(report, "Actor.mood"));
    };

    // floats
    "a number too large for a float is a warning and keeps the current value"_test = [] {
        Actor actor;
        actor.scale = 2.0f;
        Report report;
        expect(load(Ref::make(actor), R"({"scale":1e300})", &report));
        expect(actor.scale == 2.0f);   // not infinity
        expect(hasWarning(report, "Actor.scale"));

        Report negative;
        expect(load(Ref::make(actor), R"({"scale":-1e39})", &negative));
        expect(actor.scale == 2.0f);
        expect(hasWarning(negative, "Actor.scale"));
    };

    "the largest float is accepted"_test = [] {
        Actor actor;
        Report report;
        expect(load(Ref::make(actor), R"({"scale":3.4028235e38})", &report));
        expect(actor.scale == std::numeric_limits<float>::max());
        expect(report.warnings.empty());
    };

    "a double can hold what a float cannot"_test = [] {
        Actor actor;
        Report report;
        expect(load(Ref::make(actor), R"({"speed":1e300})", &report));
        expect(actor.speed == 1e300);
        expect(report.warnings.empty());
    };

    // chars
    "a char is written as a code from 0 to 255 whatever the platform's sign"_test = [] {
        Chars chars;
        chars.letter = static_cast<char>(200);
        expect(parseJson(serialize(Ref::make(chars)))["letter"] == 200);
        chars.letter = 'A';
        expect(parseJson(serialize(Ref::make(chars)))["letter"] == 65);
    };

    "every char value reads back as written"_test = [] {
        for (int code = 0; code < 256; ++code) {
            Chars source;
            source.letter = static_cast<char>(code);
            Chars copy;
            Report report;
            expect(load(Ref::make(copy), serialize(Ref::make(source)), &report));
            expect(static_cast<unsigned char>(copy.letter) == code);
            expect(report.warnings.empty());
        }
    };

    "a char code outside 0 to 255 is a warning and keeps the current value"_test = [] {
        Chars chars;
        chars.letter = 'x';
        Report tooBig;
        expect(load(Ref::make(chars), R"({"letter":256})", &tooBig));
        expect(chars.letter == 'x');
        expect(hasWarning(tooBig, "Chars.letter"));
        Report negative;
        expect(load(Ref::make(chars), R"({"letter":-1})", &negative));
        expect(chars.letter == 'x');
        expect(hasWarning(negative, "Chars.letter"));
    };

    // read-only fields
    "a read-only field is not written"_test = [] {
        Versioned value;
        value.value = 4;
        const Json json = parseJson(serialize(Ref::make(value)));
        expect(!json.contains("version"));
        expect(json["value"] == 4);
    };

    "a read-only field in the text is a warning and is left as it is"_test = [] {
        Versioned value;
        Report report;
        expect(load(Ref::make(value), R"({"version":99,"value":3})", &report));
        expect(report.ok());
        expect(value.version == 1);
        expect(value.value == 3);
        expect(hasWarning(report, "Versioned.version"));
    };

    "a struct with a read-only field reads back without a warning"_test = [] {
        Versioned source;
        source.value = 8;
        Versioned copy;
        Report report;
        expect(load(Ref::make(copy), serialize(Ref::make(source)), &report));
        expect(copy.value == 8);
        expect(report.warnings.empty());
    };
}
