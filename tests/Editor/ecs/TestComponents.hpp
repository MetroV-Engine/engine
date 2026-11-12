#pragma once

// Reflected components of every shape the Inspector and the play mode handle.
// Prefixed with "Ed": reflection names are process-wide.

#include "ECS/Reflection/Reflection.hpp"

#include <cstdint>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

enum class EdKind : std::uint8_t
{
    Minion,
    Elite,
    Boss,
};

// Scalars: bool, every integer width, floats, a string, an enum.
struct EdStats
{
    bool alive = true;
    std::int8_t level = 1;
    std::int16_t armor = 0;
    std::int32_t gold = 0;
    std::int64_t xp = 0;
    std::uint8_t a = 0;
    std::uint16_t b = 0;
    std::uint32_t c = 0;
    std::uint64_t d = 0;
    float speed = 1.0f;
    double mass = 1.0;
    long double precise = 0.0L;
    std::string title;
    EdKind kind = EdKind::Minion;
};

// Structs: a Vec2 {x, y}, a Color {r, g, b, a}, and a group of fields.
struct EdVec
{
    float x = 0.0f;
    float y = 0.0f;
};

struct EdColor
{
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
    float a = 1.0f;
};

struct EdMovement
{
    float speed = 0.0f;
    EdVec direction;
};

struct EdShape
{
    EdVec position;
    EdColor tint;
    EdMovement movement;
};

// Shown as read-only summaries.
struct EdContainers
{
    std::vector<int> path;
    std::map<std::string, int> tags;
    std::optional<int> target;
    std::pair<int, int> pair;
    char letter = 'z';
};

// Not reflected: the editor does not list it.
struct EdUnreflected
{
    int x = 0;
};

// Cannot be copied: not saved on Play.
struct EdMoveOnly
{
    EdMoveOnly() = default;
    EdMoveOnly(EdMoveOnly &&) = default;
    EdMoveOnly &operator=(EdMoveOnly &&) = default;
    EdMoveOnly(const EdMoveOnly &) = delete;
    EdMoveOnly &operator=(const EdMoveOnly &) = delete;
    int value = 0;
};

// Cannot be added from the editor.
struct EdNoDefault
{
    explicit EdNoDefault(int v) : value(v) {}
    int value;
};

// Throws on demand when constructed or copied.
struct EdFragile
{
    static inline bool failConstruct = false;
    static inline bool failCopy = false;

    int value = 0;

    EdFragile()
    {
        if (failConstruct)
            throw std::runtime_error("construction failed");
    }
    EdFragile(const EdFragile &other) : value(other.value)
    {
        if (failCopy)
            throw std::runtime_error("copy failed");
    }
    EdFragile(EdFragile &&other) noexcept : value(other.value) {}
    EdFragile &operator=(const EdFragile &) = default;
    EdFragile &operator=(EdFragile &&) noexcept = default;
};

E_REFLECT(EdStats)
    E_FIELD(alive, bool)
    E_FIELD(level, std::int8_t)
    E_FIELD(armor, std::int16_t)
    E_FIELD(gold, std::int32_t)
    E_FIELD(xp, std::int64_t)
    E_FIELD(a, std::uint8_t)
    E_FIELD(b, std::uint16_t)
    E_FIELD(c, std::uint32_t)
    E_FIELD(d, std::uint64_t)
    E_FIELD(speed, float)
    E_FIELD(mass, double)
    E_FIELD(precise, long double)
    E_FIELD(title, std::string)
    E_FIELD(kind, EdKind)
E_END

E_REFLECT(EdVec)
    E_FIELD(x, float)
    E_FIELD(y, float)
E_END

E_REFLECT(EdColor)
    E_FIELD(r, float)
    E_FIELD(g, float)
    E_FIELD(b, float)
    E_FIELD(a, float)
E_END

E_REFLECT(EdMovement)
    E_FIELD(speed, float)
    E_FIELD(direction, EdVec)
E_END

E_REFLECT(EdShape)
    E_FIELD(position, EdVec)
    E_FIELD(tint, EdColor)
    E_FIELD(movement, EdMovement)
E_END

E_REFLECT(EdContainers)
    E_FIELD(path, std::vector<int>)
    E_FIELD(tags, std::map<std::string, int>)
    E_FIELD(target, std::optional<int>)
    E_FIELD(pair, std::pair<int, int>)
    E_FIELD(letter, char)
E_END

E_REFLECT(EdMoveOnly)
    E_FIELD(value, int)
E_END

E_REFLECT(EdNoDefault)
    E_FIELD(value, int)
E_END

E_REFLECT(EdFragile)
    E_FIELD(value, int)
E_END
