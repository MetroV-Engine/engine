#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <optional>
#include <random>
#include <string>
#include <string_view>

#include "ECS/Reflection/Descriptor.hpp"
#include "ECS/Reflection/TypeRegistry.hpp"   // defines the registration E_REFLECT calls

/**
 * @brief Persistent identity of an entity, stored as a regular component.
 *
 * An Entity handle is only valid inside the Registry that issued it: its
 * index, generation and WorldId all change when a world is reloaded. A
 * Uuid is what survives: scene files identify entities by it, and
 * references between entities are saved as the Uuid of their target.
 *
 * Like every component, it is plain data: 128 bits, compared and hashed
 * by value. Creating, printing and parsing one is done by the functions
 * of the `uuid` namespace below.
 *
 * Every entity created by the editor has one: saving a scene gives one to
 * those that lack it (SaveOptions::assignMissingUuids). Entities spawned
 * while a game runs need none, since only the editor's scenes are saved
 * for now. A Uuid is unique inside a world and must never be modified once
 * the entity has it. Copying an entity must give the copy a
 * new Uuid (uuid::generate()).
 */
struct Uuid {
    std::uint64_t high = 0;
    std::uint64_t low = 0;

    friend bool operator==(const Uuid&, const Uuid&) = default;
    friend auto operator<=>(const Uuid&, const Uuid&) = default;
};

/**
 * @brief Operations on a Uuid. The text form is the usual
 *        "xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx", in lower case.
 */
namespace uuid {
    /** @brief True for the all-zero value, which means "no identity". */
    [[nodiscard]] inline bool isNull(const Uuid& id) noexcept {
        return id.high == 0 && id.low == 0;
    }

    /** @brief A new random (version 4), non-null Uuid. Thread-safe. */
    [[nodiscard]] inline Uuid generate() {
        thread_local std::mt19937_64 engine = [] {
            std::random_device device;
            const std::uint64_t seed =
                (static_cast<std::uint64_t>(device()) << 32) ^ static_cast<std::uint64_t>(device());
            return std::mt19937_64(seed);
        }();
        Uuid id;
        id.high = (engine() & 0xFFFFFFFFFFFF0FFFull) | 0x0000000000004000ull;   // version 4
        id.low = (engine() & 0x3FFFFFFFFFFFFFFFull) | 0x8000000000000000ull;    // variant 1
        return id;
    }

    /** @brief The 36-character text form, in lower case. */
    [[nodiscard]] inline std::string toString(const Uuid& id) {
        char text[37];
        std::snprintf(text, sizeof(text), "%08x-%04x-%04x-%04x-%012llx",
                      static_cast<unsigned>(id.high >> 32),
                      static_cast<unsigned>((id.high >> 16) & 0xFFFF),
                      static_cast<unsigned>(id.high & 0xFFFF),
                      static_cast<unsigned>(id.low >> 48),
                      static_cast<unsigned long long>(id.low & 0xFFFFFFFFFFFFull));
        return text;
    }

    /**
     * @brief Parses the text form, upper or lower case.
     * @return nullopt when text is not exactly 8-4-4-4-12 hexadecimal digits.
     */
    [[nodiscard]] inline std::optional<Uuid> fromString(std::string_view text) {
        if (text.size() != 36) {
            return std::nullopt;
        }
        Uuid id;
        int digits = 0;
        for (std::size_t i = 0; i < text.size(); ++i) {
            if (i == 8 || i == 13 || i == 18 || i == 23) {
                if (text[i] != '-') {
                    return std::nullopt;
                }
                continue;
            }
            const char c = text[i];
            std::uint64_t nibble = 0;
            if (c >= '0' && c <= '9') {
                nibble = static_cast<std::uint64_t>(c - '0');
            } else if (c >= 'a' && c <= 'f') {
                nibble = static_cast<std::uint64_t>(c - 'a' + 10);
            } else if (c >= 'A' && c <= 'F') {
                nibble = static_cast<std::uint64_t>(c - 'A' + 10);
            } else {
                return std::nullopt;
            }
            std::uint64_t& half = digits < 16 ? id.high : id.low;
            half = (half << 4) | nibble;
            ++digits;
        }
        return id;
    }
}

template<>
struct std::hash<Uuid> {
    std::size_t operator()(const Uuid& id) const noexcept {
        return std::hash<std::uint64_t>{}(id.high ^ (id.low * 0x9E3779B97F4A7C15ull + (id.high << 6) + (id.high >> 2)));
    }
};

// Reflected here, in the header, and not in a .cpp: E_REFLECT registers the
// type through a static variable, so every program that includes Uuid.hpp
// registers it. A .cpp of the static engine_core that nothing references
// would be dropped by the linker, and the type never registered.
E_REFLECT(Uuid)
    E_FIELD(high, std::uint64_t)
    E_FIELD(low, std::uint64_t)
E_END
