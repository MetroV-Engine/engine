/*
    Created on: 10/11/2026
    Filename: LevelFile.cpp
    Description: Writes a scene as a level file, and reads it back

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "LevelFile.hpp"

#include <algorithm>
#include <charconv>
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <stdexcept>
#include <system_error>
#include <type_traits>
#include <utility>
#include <variant>

namespace {

using ObjectId = HierarchyModel::ObjectId;

constexpr const char *Header = "metrov-level";

// ---- Writing

std::string quote(const std::string &text)
{
    std::string quoted = "\"";
    for (const char c : text) {
        if (c == '"' || c == '\\')
            quoted += '\\';
        if (c == '\n')
            quoted += "\\n";
        else
            quoted += c;
    }
    return quoted + '"';
}

std::string number(float value)
{
    // Enough digits for the float to read back exactly.
    char text[32];
    std::snprintf(text, sizeof(text), "%.9g", value);
    return text;
}

// The type and value written after a property's path; empty if not saved.
std::string valueText(const PropertyValue &value)
{
    return std::visit(
        [](const auto &v) -> std::string {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, bool>)
                return std::string("bool ") + (v ? "true" : "false");
            else if constexpr (std::is_same_v<T, int>)
                return "int " + std::to_string(v);
            else if constexpr (std::is_same_v<T, float>)
                return "float " + number(v);
            else if constexpr (std::is_same_v<T, std::string>)
                return "string " + quote(v);
            else if constexpr (std::is_same_v<T, Vec2>)
                return "vec2 " + number(v.x) + " " + number(v.y);
            else if constexpr (std::is_same_v<T, Color>)
                return "color " + number(v.r) + " " + number(v.g) + " " + number(v.b) + " " + number(v.a);
            else
                return {};
        },
        value);
}

class Writer
{
public:
    Writer(const HierarchyModel &hierarchy, const PropertyModel &properties) : _hierarchy(hierarchy), _properties(properties) {}

    std::string write()
    {
        _out = LevelFile::empty();
        for (ObjectId root : _hierarchy.roots())
            writeObject(root, -1);
        return std::move(_out);
    }

private:
    const HierarchyModel &_hierarchy;
    const PropertyModel &_properties;
    std::string _out;
    int _next = 0;

    // Display order, each object before its children.
    void writeObject(ObjectId id, int parent)
    {
        const int index = _next++;
        _out += "object " + std::to_string(index) + " " + (parent < 0 ? "-" : std::to_string(parent)) + " "
            + quote(_hierarchy.name(id)) + "\n";
        for (const ComponentInfo &component : _properties.components(id)) {
            _out += "component " + quote(component.name) + "\n";
            writeProperties(id, component.name, component.properties, "");
        }
        for (ObjectId child : _hierarchy.children(id))
            writeObject(child, index);
    }

    void writeProperties(ObjectId id, const std::string &component, const std::vector<PropertyInfo> &infos, const std::string &prefix)
    {
        for (const PropertyInfo &info : infos) {
            if (info.readOnly)
                continue;
            const std::string path = prefix + info.name;
            if (info.type == PropertyType::Group) {
                writeProperties(id, component, info.fields, path + ".");
                continue;
            }
            const std::string value = valueText(_properties.get(id, component, path));
            if (!value.empty())
                _out += "property " + quote(path) + " " + value + "\n";
        }
    }
};

// ---- Reading

struct LevelComponent
{
    std::string name;
    std::vector<std::pair<std::string, PropertyValue>> properties;
};

struct LevelObject
{
    std::string name;
    // Index of an earlier object, or -1 for a root.
    int parent;
    std::vector<LevelComponent> components;
};

std::runtime_error error(int line, const std::string &message)
{
    return std::runtime_error("line " + std::to_string(line) + ": " + message);
}

bool isSpace(char c)
{
    return c == ' ' || c == '\t' || c == '\r';
}

// The words of a line; a quoted string is one word, unescaped.
std::vector<std::string> words(const std::string &line, int number)
{
    std::vector<std::string> result;
    std::size_t i = 0;
    while (i < line.size()) {
        if (isSpace(line[i])) {
            ++i;
            continue;
        }
        std::string word;
        if (line[i] != '"') {
            while (i < line.size() && !isSpace(line[i]))
                word += line[i++];
            result.push_back(std::move(word));
            continue;
        }
        ++i;
        bool closed = false;
        while (i < line.size() && !closed) {
            const char c = line[i++];
            if (c == '"') {
                closed = true;
            } else if (c == '\\' && i < line.size()) {
                word += line[i] == 'n' ? '\n' : line[i];
                ++i;
            } else {
                word += c;
            }
        }
        if (!closed)
            throw error(number, "unclosed quote");
        result.push_back(std::move(word));
    }
    return result;
}

int toInt(const std::string &text, int line)
{
    // Out of range is refused, not cut down to an int (4294967296 is not 0).
    int value = 0;
    const char *end = text.data() + text.size();
    const auto [stop, failure] = std::from_chars(text.data(), end, value);
    if (text.empty() || failure != std::errc() || stop != end)
        throw error(line, "\"" + text + "\" is not an integer");
    return value;
}

float toFloat(const std::string &text, int line)
{
    char *end = nullptr;
    const float value = std::strtof(text.c_str(), &end);
    if (text.empty() || *end != '\0')
        throw error(line, "\"" + text + "\" is not a number");
    return value;
}

// The value of a property line: its type at w[at], then its values.
PropertyValue parseValue(const std::vector<std::string> &w, std::size_t at, int line)
{
    const std::string &type = w[at];
    const auto expect = [&](std::size_t count) {
        if (w.size() != at + 1 + count)
            throw error(line, type + " expects " + std::to_string(count) + (count == 1 ? " value" : " values"));
    };
    const auto f = [&](std::size_t i) { return toFloat(w[at + i], line); };
    if (type == "bool") {
        expect(1);
        if (w[at + 1] != "true" && w[at + 1] != "false")
            throw error(line, "bool expects true or false");
        return w[at + 1] == "true";
    }
    if (type == "int") {
        expect(1);
        return toInt(w[at + 1], line);
    }
    if (type == "float") {
        expect(1);
        return f(1);
    }
    if (type == "string") {
        expect(1);
        return w[at + 1];
    }
    if (type == "vec2") {
        expect(2);
        return Vec2{f(1), f(2)};
    }
    if (type == "color") {
        expect(4);
        return Color{f(1), f(2), f(3), f(4)};
    }
    throw error(line, "unknown type \"" + type + "\"");
}

std::vector<LevelObject> parse(const std::string &text)
{
    std::vector<LevelObject> objects;
    std::istringstream lines(text);
    std::string line;
    int number = 0;
    bool header = false;
    while (std::getline(lines, line)) {
        ++number;
        const std::vector<std::string> w = words(line, number);
        if (w.empty())
            continue;
        if (!header) {
            if (w.size() != 2 || w[0] != Header || w[1] != std::to_string(LevelFile::Version))
                throw error(number, std::string("not a level: expected \"") + Header + " " + std::to_string(LevelFile::Version) + "\"");
            header = true;
        } else if (w[0] == "object") {
            if (w.size() != 4)
                throw error(number, "expected: object <index> <parent index or -> \"<name>\"");
            if (toInt(w[1], number) != static_cast<int>(objects.size()))
                throw error(number, "objects must be numbered in order from 0");
            const int parent = w[2] == "-" ? -1 : toInt(w[2], number);
            if (parent < -1 || parent >= static_cast<int>(objects.size()))
                throw error(number, "a parent must come before its children");
            objects.push_back({w[3], parent, {}});
        } else if (w[0] == "component") {
            if (w.size() != 2 || objects.empty())
                throw error(number, "expected: component \"<name>\", after an object");
            objects.back().components.push_back({w[1], {}});
        } else if (w[0] == "property") {
            if (w.size() < 3 || objects.empty() || objects.back().components.empty())
                throw error(number, "expected: property \"<path>\" <type> <values>, after a component");
            objects.back().components.back().properties.emplace_back(w[1], parseValue(w, 2, number));
        } else {
            throw error(number, "unknown line \"" + w[0] + "\"");
        }
    }
    if (!header)
        throw std::runtime_error("empty file: not a level");
    return objects;
}

bool hasComponent(const PropertyModel &properties, ObjectId id, const std::string &name)
{
    return std::ranges::any_of(properties.components(id), [&](const ComponentInfo &c) { return c.name == name; });
}

} // namespace

std::string LevelFile::write(const HierarchyModel &hierarchy, const PropertyModel &properties)
{
    return Writer(hierarchy, properties).write();
}

std::string LevelFile::empty()
{
    return std::string(Header) + " " + std::to_string(Version) + "\n";
}

std::vector<std::string> LevelFile::read(const std::string &text, HierarchyModel &hierarchy, PropertyModel &properties)
{
    // Parsed first: an invalid file must not empty the scene.
    const std::vector<LevelObject> objects = parse(text);
    for (ObjectId root : hierarchy.roots())
        hierarchy.destroy(root);

    std::vector<std::string> problems;
    std::vector<ObjectId> created;
    bool parentsIgnored = false;
    for (const LevelObject &object : objects) {
        ObjectId parent = HierarchyModel::InvalidId;
        if (object.parent >= 0 && hierarchy.supportsParenting())
            parent = created[object.parent];
        parentsIgnored |= object.parent >= 0 && !hierarchy.supportsParenting();

        const ObjectId id = hierarchy.create(parent, object.name);
        created.push_back(id);
        if (id == HierarchyModel::InvalidId) {
            problems.push_back("Cannot create \"" + object.name + "\"");
            continue;
        }
        for (const LevelComponent &component : object.components) {
            // Some scenes give every object a component (the demo's Transform).
            if (!properties.addComponent(id, component.name) && !hasComponent(properties, id, component.name)) {
                problems.push_back("Cannot add " + component.name + " to \"" + object.name + "\"");
                continue;
            }
            for (const auto &[path, value] : component.properties)
                if (!properties.set(id, component.name, path, value))
                    problems.push_back("Cannot set " + component.name + "." + path + " of \"" + object.name + "\"");
        }
    }
    if (parentsIgnored)
        problems.push_back("This scene is flat: the objects were created without their parents");
    return problems;
}
