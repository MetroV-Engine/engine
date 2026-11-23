#pragma once

// Reflected types shared by the serialization tests. They live in a named
// namespace (not an anonymous one) so that every test file including this
// header sees the same type, registered once under the same name.

#include <array>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "ECS/ECS.hpp"
#include "Engine/Components/Uuid.hpp"
#include "ECS/Reflection/Reflection.hpp"

namespace sertest {
    struct Health {
        int current = 100;
        int max = 100;
    };

    struct Item {
        std::string id;
        int count = 1;
    };

    enum class Mood : int { Calm = 0, Angry = 7 };

    struct Actor {
        Mood mood = Mood::Calm;
        double speed = 1.5;
        float scale = 1.0f;
        bool alive = true;
        std::uint8_t level = 1;
        std::int64_t big = 0;
        std::string title;
    };

    struct Inventory {
        std::vector<Item> items;
        std::optional<Item> equipped;
        std::map<std::string, int> counts;
        std::unordered_map<std::string, int> loose;
        std::map<int, std::string> byNumber;
        std::array<int, 3> slots{};
    };

    struct Link {
        ECS::Entity target;
        std::vector<ECS::Entity> others;
    };

    struct Owner {
        Uuid asset;
    };

    struct Blob {   // not reflected: Opaque
        int a = 0;
    };

    struct WithBlob {
        int kept = 1;
        Blob blob;
    };

    struct NoDefault {
        explicit NoDefault(int v) : value(v) {}
        int value;
    };

    struct Marker {};   // a component with no field

    enum class Small : std::uint8_t { Low = 0, High = 200 };

    struct Sized {
        Small small = Small::Low;
    };

    struct Chars {
        char letter = 0;
    };

    struct Versioned {   // a const member: readable, never writable
        const int version = 1;
        int value = 0;
    };
}

E_REFLECT(sertest::Health)
    E_FIELD(current, int)
    E_FIELD(max, int)
E_END

E_REFLECT(sertest::Item)
    E_FIELD(id, std::string)
    E_FIELD(count, int)
E_END

E_REFLECT(sertest::Actor)
    E_FIELD(mood, sertest::Mood)
    E_FIELD(speed, double)
    E_FIELD(scale, float)
    E_FIELD(alive, bool)
    E_FIELD(level, std::uint8_t)
    E_FIELD(big, std::int64_t)
    E_FIELD(title, std::string)
E_END

E_REFLECT(sertest::Inventory)
    E_FIELD(items, std::vector<sertest::Item>)
    E_FIELD(equipped, std::optional<sertest::Item>)
    E_FIELD(counts, std::map<std::string, int>)
    E_FIELD(loose, std::unordered_map<std::string, int>)
    E_FIELD(byNumber, std::map<int, std::string>)
    E_FIELD(slots, std::array<int, 3>)
E_END

E_REFLECT(sertest::Link)
    E_FIELD(target, ECS::Entity)
    E_FIELD(others, std::vector<ECS::Entity>)
E_END

E_REFLECT(sertest::Owner)
    E_FIELD(asset, Uuid)
E_END

E_REFLECT(sertest::WithBlob)
    E_FIELD(kept, int)
    E_FIELD(blob, sertest::Blob)
E_END

E_REFLECT(sertest::NoDefault)
    E_FIELD(value, int)
E_END

E_REFLECT(sertest::Marker)
E_END

E_REFLECT(sertest::Sized)
    E_FIELD(small, sertest::Small)
E_END

E_REFLECT(sertest::Chars)
    E_FIELD(letter, char)
E_END

E_REFLECT(sertest::Versioned)
    E_FIELD(version, const int)
    E_FIELD(value, int)
E_END
