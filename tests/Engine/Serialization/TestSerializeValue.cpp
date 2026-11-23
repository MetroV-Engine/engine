#include <boost/ut.hpp>

#include <cmath>
#include <cstdint>
#include <limits>
#include <string>

#include "Engine/Serialization/Serialization.hpp"
#include "Serialization/Support/Helpers.hpp"
#include "Serialization/Support/TestTypes.hpp"

using namespace boost::ut;
using ECS::reflect::Ref;
using serial::Report;
using serial::serialize;
using namespace sertest;

void run_serialize_value_tests() {
    // ---- structs and scalars

    "a struct is written as an object of its fields"_test = [] {
        Health health{75, 100};
        Report report;
        const std::string text = serialize(Ref::make(health), &report);
        expect(report.ok());
        expect(report.warnings.empty());
        expect(parseJson(text) == parseJson(R"({"current":75,"max":100})"));
    };

    "fields are written in declaration order"_test = [] {
        Health health;
        const std::string text = serialize(Ref::make(health));
        expect(text.find("current") < text.find("max"));
    };

    "the text ends with a newline"_test = [] {
        Health health;
        const std::string text = serialize(Ref::make(health));
        expect(!text.empty() && text.back() == '\n');
    };

    "a struct without a field is an empty object"_test = [] {
        Marker marker;
        expect(parseJson(serialize(Ref::make(marker))) == Json::object());
    };

    "a scalar is written as a plain value"_test = [] {
        int number = 5;
        std::string text = "abc";
        bool flag = true;
        expect(parseJson(serialize(Ref::make(number))) == Json(5));
        expect(parseJson(serialize(Ref::make(text))) == Json("abc"));
        expect(parseJson(serialize(Ref::make(flag))) == Json(true));
    };

    "every scalar kind keeps its value"_test = [] {
        Actor actor;
        actor.mood = Mood::Angry;
        actor.speed = 2.5;
        actor.alive = false;
        actor.level = 255;
        actor.big = std::numeric_limits<std::int64_t>::min();
        actor.title = "caf\xC3\xA9 \"quoted\"\n";
        const Json json = parseJson(serialize(Ref::make(actor)));
        expect(json["mood"] == 7);
        expect(json["speed"] == 2.5);
        expect(json["alive"] == false);
        expect(json["level"] == 255);
        expect(json["big"] == std::numeric_limits<std::int64_t>::min());
        expect(json["title"] == actor.title);
    };

    "an unsigned 64-bit value above the signed range is kept"_test = [] {
        std::uint64_t big = std::numeric_limits<std::uint64_t>::max();
        expect(serialize(Ref::make(big)) == "18446744073709551615\n");
    };

    "a float is written with its shortest text"_test = [] {
        Actor actor;
        actor.scale = 0.1f;
        const std::string text = serialize(Ref::make(actor));
        expect(text.find("0.1,") != std::string::npos || text.find("0.1\n") != std::string::npos);
        expect(text.find("0.10000") == std::string::npos);
    };

    "an enum is written as its integer value"_test = [] {
        Mood mood = Mood::Angry;
        expect(parseJson(serialize(Ref::make(mood))) == Json(7));
    };

    // ---- containers

    "a vector is written as an array"_test = [] {
        Inventory inventory;
        inventory.items = {{"sword", 1}, {"shield", 2}};
        const Json json = parseJson(serialize(Ref::make(inventory)));
        expect(json["items"].size() == std::size_t{2});
        expect(json["items"][1]["id"] == "shield");
        expect(json["items"][1]["count"] == 2);
    };

    "an empty vector is an empty array"_test = [] {
        Inventory inventory;
        expect(parseJson(serialize(Ref::make(inventory)))["items"] == Json::array());
    };

    "a fixed array is written as an array"_test = [] {
        Inventory inventory;
        inventory.slots = {1, 2, 3};
        expect(parseJson(serialize(Ref::make(inventory)))["slots"] == parseJson("[1,2,3]"));
    };

    "an empty optional is null and a filled one is its value"_test = [] {
        Inventory inventory;
        expect(parseJson(serialize(Ref::make(inventory)))["equipped"].is_null());
        inventory.equipped = Item{"bow", 1};
        expect(parseJson(serialize(Ref::make(inventory)))["equipped"]["id"] == "bow");
    };

    "a map is written as an object"_test = [] {
        Inventory inventory;
        inventory.counts = {{"gold", 5}, {"arrow", 20}};
        const Json json = parseJson(serialize(Ref::make(inventory)));
        expect(json["counts"]["gold"] == 5);
        expect(json["counts"]["arrow"] == 20);
    };

    "map keys are written sorted, for a map and an unordered map"_test = [] {
        Inventory inventory;
        inventory.counts = {{"b", 2}, {"a", 1}, {"c", 3}};
        inventory.loose = {{"z", 1}, {"m", 2}, {"a", 3}, {"q", 4}};
        const std::string text = serialize(Ref::make(inventory));
        expect(text.find("\"a\": 1") < text.find("\"b\": 2"));
        expect(text.find("\"b\": 2") < text.find("\"c\": 3"));
        expect(text.find("\"a\": 3") < text.find("\"m\": 2"));
        expect(text.find("\"m\": 2") < text.find("\"q\": 4"));
        expect(text.find("\"q\": 4") < text.find("\"z\": 1"));
    };

    "integer map keys are written as text"_test = [] {
        Inventory inventory;
        inventory.byNumber = {{2, "two"}, {10, "ten"}};
        const Json json = parseJson(serialize(Ref::make(inventory)));
        expect(json["byNumber"]["2"] == "two");
        expect(json["byNumber"]["10"] == "ten");
    };

    "an empty map is an empty object"_test = [] {
        Inventory inventory;
        expect(parseJson(serialize(Ref::make(inventory)))["counts"] == Json::object());
    };

    // ---- what cannot be written

    "a non-finite number is written as null with a warning"_test = [] {
        Actor actor;
        actor.speed = std::numeric_limits<double>::infinity();
        actor.scale = std::numeric_limits<float>::quiet_NaN();
        Report report;
        const Json json = parseJson(serialize(Ref::make(actor), &report));
        expect(report.ok());
        expect(json["speed"].is_null());
        expect(json["scale"].is_null());
        expect(hasWarning(report, "Actor.speed"));
        expect(hasWarning(report, "Actor.scale"));
    };

    "an opaque field is skipped with a warning"_test = [] {
        WithBlob value;
        Report report;
        const Json json = parseJson(serialize(Ref::make(value), &report));
        expect(report.ok());
        expect(json.contains("kept"));
        expect(!json.contains("blob"));
        expect(hasWarning(report, "WithBlob.blob"));
    };

    "an opaque value alone is an error"_test = [] {
        Blob blob;
        Report report;
        expect(serialize(Ref::make(blob), &report).empty());
        expect(!report.ok());
    };

    "a string that is not valid UTF-8 is an error, not a corrupted text"_test = [] {
        Item item;
        item.id = "\xFF\xFE";
        Report report;
        expect(serialize(Ref::make(item), &report).empty());
        expect(!report.ok());
    };

    "an invalid Ref is an error"_test = [] {
        Report report;
        expect(serialize(Ref{}, &report).empty());
        expect(!report.ok());
    };

    "serialize works without a report"_test = [] {
        Health health;
        expect(!serialize(Ref::make(health)).empty());
        expect(serialize(Ref{}).empty());
    };

    "a report is appended to, never cleared"_test = [] {
        Report report;
        report.warnings.push_back("earlier");
        Health health;
        (void)serialize(Ref::make(health), &report);
        expect(report.warnings.size() == std::size_t{1});
    };

    // ---- entities and ids as values

    "an entity reference alone cannot be resolved: null with a warning"_test = [] {
        ECS::Registry world;
        Link link;
        link.target = world.spawnEntity();
        Report report;
        const Json json = parseJson(serialize(Ref::make(link), &report));
        expect(json["target"].is_null());
        expect(hasWarning(report, "Link.target"));
    };

    "a handle that refers to nothing is null without a warning"_test = [] {
        Link link;
        Report report;
        const Json json = parseJson(serialize(Ref::make(link), &report));
        expect(json["target"].is_null());
        expect(report.warnings.empty());
    };

    "a Uuid field is written as its text"_test = [] {
        Owner owner;
        owner.asset = Uuid{0x0123456789ABCDEFull, 0xFEDCBA9876543210ull};
        const Json json = parseJson(serialize(Ref::make(owner)));
        expect(json["asset"] == "01234567-89ab-cdef-fedc-ba9876543210");
    };
}
