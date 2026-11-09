/*
    Created on: 30/03/2026
    Filename: PropertyModel.hpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#pragma once

#include "scene/HierarchyModel.hpp"

#include <string>
#include <variant>
#include <vector>

struct Vec2
{
    float x = 0.0f;
    float y = 0.0f;
    bool operator==(const Vec2 &) const = default;
};

struct Color
{
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
    float a = 1.0f;
    bool operator==(const Color &) const = default;
};

// A property value. std::monostate means "no value" (unknown property).
// Enums are stored as their option index (int).
using PropertyValue = std::variant<std::monostate, bool, int, float, std::string, Vec2, Color>;

enum class PropertyType
{
    Bool,
    Int,
    Float,
    String,
    Vec2,
    Color,
    Enum,  // int index into PropertyInfo::enumOptions
    Group, // nested properties (a struct field), see PropertyInfo::fields
};

// Description of one editable property, used by the Inspector to pick a widget.
// It mirrors the engine's reflection (FieldInfo / TypeKind): EcsScene builds it
// from reflected components.
struct PropertyInfo
{
    std::string name;
    PropertyType type = PropertyType::Float;
    bool readOnly = false;
    // Numeric bounds; min == max means unbounded.
    float min = 0.0f;
    float max = 0.0f;
    // Drag speed of numeric fields, per pixel.
    float speed = 0.1f;
    // printf format of numeric fields (empty: default).
    std::string format;
    std::string tooltip;
    std::vector<std::string> enumOptions;
    std::vector<PropertyInfo> fields;
};

// A component of an object (Transform, Sprite...) and its properties.
struct ComponentInfo
{
    std::string name;
    std::vector<PropertyInfo> properties;
    // Core components (e.g. Transform) cannot be removed.
    bool removable = true;
};

// The property the mouse writes in the Viewport; objects without it cannot be placed.
inline constexpr const char *PositionComponent = "Transform";
inline constexpr const char *PositionProperty = "position";

// The data shown by the Inspector: components of an object and their
// properties. Properties are addressed by a path inside their component, like
// the engine's Ref::path(): "speed", or "movement.speed" inside a Group.
class PropertyModel
{
public:
    using ObjectId = HierarchyModel::ObjectId;

    virtual ~PropertyModel() = default;

    // Components of the object, in display order (empty if it does not exist).
    virtual std::vector<ComponentInfo> components(ObjectId id) const = 0;
    // std::monostate if the object, component or property does not exist.
    virtual PropertyValue get(ObjectId id, const std::string &component, const std::string &path) const = 0;
    // Returns false if refused (unknown property, read-only, wrong type).
    virtual bool set(ObjectId id, const std::string &component, const std::string &path, const PropertyValue &value) = 0;

    // Component types that can still be added to the object.
    virtual std::vector<std::string> addableComponents(ObjectId id) const = 0;
    virtual bool addComponent(ObjectId id, const std::string &component) = 0;
    virtual bool removeComponent(ObjectId id, const std::string &component) = 0;
};
