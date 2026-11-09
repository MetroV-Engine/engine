/*
    Created on: 03/11/2026
    Filename: EcsScene.cpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "EcsScene.hpp"
#include "core/Logger/Logger.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <exception>
#include <initializer_list>
#include <limits>
#include <sstream>
#include <string_view>
#include <utility>

namespace {

namespace reflect = ECS::reflect;
using reflect::Ref;
using reflect::TypeInfo;
using reflect::TypeKind;

// ObjectId layout: generation in the high 32 bits, index + 1 in the low 32
// bits, so that no entity gets InvalidId (0).
constexpr std::uint64_t IndexMask = 0xFFFFFFFFull;
constexpr std::size_t IndexLimit = static_cast<std::size_t>(IndexMask);

const TypeInfo *nameType()
{
    return reflect::TypeRegistry::instance().find<ECS::Name>();
}

// Integer conversions saturate instead of wrapping.
template <typename To, typename From>
To clampTo(From value)
{
    if (std::cmp_less(value, std::numeric_limits<To>::min()))
        return std::numeric_limits<To>::min();
    if (std::cmp_greater(value, std::numeric_limits<To>::max()))
        return std::numeric_limits<To>::max();
    return static_cast<To>(value);
}

template <typename From>
float toFloat(From value)
{
    if (!std::isfinite(value))
        return static_cast<float>(value);
    return static_cast<float>(std::clamp<From>(value, -std::numeric_limits<float>::max(), std::numeric_limits<float>::max()));
}

// An integer type edited as an int. Aliases (long / long long...) share the
// TypeInfo of one of the fixed-width types, so these cover every integer.
struct IntegerType
{
    const TypeInfo *type;
    // Drag bounds of the Inspector: the type's range when narrower than int,
    // otherwise unbounded (0, 0); writes are clamped either way.
    float min;
    float max;
    int (*read)(const void *value);
    void (*write)(void *value, int number);
    // number clamped to the type's range.
    int (*clamp)(int number);
};

template <typename T>
IntegerType integerType()
{
    constexpr bool narrow = sizeof(T) < sizeof(int);
    return {reflect::TypeRegistry::instance().find<T>(),
        narrow ? static_cast<float>(std::numeric_limits<T>::min()) : 0.0f,
        narrow ? static_cast<float>(std::numeric_limits<T>::max()) : 0.0f,
        [](const void *value) { return clampTo<int>(*static_cast<const T *>(value)); },
        [](void *value, int number) { *static_cast<T *>(value) = clampTo<T>(number); },
        [](int number) { return clampTo<int>(clampTo<T>(number)); }};
}

const IntegerType *findInteger(const TypeInfo &type)
{
    static const IntegerType types[] = {integerType<std::int8_t>(), integerType<std::int16_t>(),
        integerType<std::int32_t>(), integerType<std::int64_t>(), integerType<std::uint8_t>(),
        integerType<std::uint16_t>(), integerType<std::uint32_t>(), integerType<std::uint64_t>()};
    for (const IntegerType &integer : types)
        if (integer.type == &type)
            return &integer;
    return nullptr;
}

// A floating-point type edited as a float.
struct FloatType
{
    const TypeInfo *type;
    float (*read)(const void *value);
    void (*write)(void *value, float number);
};

template <typename T>
FloatType floatType()
{
    return {reflect::TypeRegistry::instance().find<T>(),
        [](const void *value) { return toFloat(*static_cast<const T *>(value)); },
        [](void *value, float number) { *static_cast<T *>(value) = static_cast<T>(number); }};
}

const FloatType *findFloat(const TypeInfo &type)
{
    static const FloatType types[] = {floatType<float>(), floatType<double>(), floatType<long double>()};
    for (const FloatType &floating : types)
        if (floating.type == &type)
            return &floating;
    return nullptr;
}

// How a value is shown in the Inspector.
enum class Shape
{
    Bool,
    Int,
    Float,
    String,
    Enum,
    Vec2,
    Color,
    Group,
    Text, // Not editable: shown as a read-only summary
};

// A struct made of exactly these float fields, in this order.
bool hasFloatFields(const TypeInfo &type, std::initializer_list<std::string_view> names)
{
    if (type.kind != TypeKind::Struct || type.fields.size() != names.size())
        return false;
    const reflect::TypeInfo *floating = reflect::TypeRegistry::instance().find<float>();
    std::size_t i = 0;
    for (const std::string_view name : names) {
        const reflect::FieldInfo &field = type.fields[i++];
        if (field.name != name || field.type != floating)
            return false;
    }
    return true;
}

Shape shapeOf(const TypeInfo &type)
{
    if (&type == reflect::TypeRegistry::instance().find<bool>())
        return Shape::Bool;
    if (findInteger(type))
        return Shape::Int;
    if (findFloat(type))
        return Shape::Float;
    if (&type == reflect::TypeRegistry::instance().find<std::string>())
        return Shape::String;
    if (type.kind == TypeKind::Enum)
        return Shape::Enum;
    if (hasFloatFields(type, {"x", "y"}))
        return Shape::Vec2;
    if (hasFloatFields(type, {"r", "g", "b", "a"}))
        return Shape::Color;
    if (type.kind == TypeKind::Struct)
        return Shape::Group;
    return Shape::Text;
}

// A Vec2 or Color edited as a whole cannot be written if one of its members is const.
bool hasConstMember(const TypeInfo &type)
{
    return std::ranges::any_of(type.fields, &reflect::FieldInfo::readOnly);
}

PropertyInfo describeField(const reflect::FieldInfo &field, bool parentReadOnly)
{
    const TypeInfo &type = *field.type;
    PropertyInfo info;
    info.name = field.name;
    info.readOnly = parentReadOnly || field.readOnly;
    info.tooltip = type.name;

    switch (shapeOf(type)) {
    case Shape::Bool:
        info.type = PropertyType::Bool;
        break;
    case Shape::Int: {
        const IntegerType &integer = *findInteger(type);
        info.type = PropertyType::Int;
        info.min = integer.min;
        info.max = integer.max;
        info.speed = 0.2f;
        break;
    }
    case Shape::Float:
        info.type = PropertyType::Float;
        break;
    case Shape::String:
        info.type = PropertyType::String;
        break;
    case Shape::Enum:
        info.type = PropertyType::Int;
        info.speed = 0.05f;
        info.tooltip = "Enum, edited as its integer value";
        break;
    case Shape::Vec2:
        info.type = PropertyType::Vec2;
        info.readOnly = info.readOnly || hasConstMember(type);
        break;
    case Shape::Color:
        info.type = PropertyType::Color;
        info.readOnly = info.readOnly || hasConstMember(type);
        break;
    case Shape::Group:
        info.type = PropertyType::Group;
        for (const reflect::FieldInfo &member : type.fields)
            info.fields.push_back(describeField(member, info.readOnly));
        break;
    case Shape::Text:
        info.type = PropertyType::String;
        info.readOnly = true;
        info.tooltip = type.name + ": not editable in the Inspector yet";
        return info;
    }
    if (info.readOnly && info.type != PropertyType::Group)
        info.tooltip += " (read-only: const member)";
    return info;
}

float floatMember(const Ref &value, std::size_t index)
{
    return value.field(value.type().fields[index]).as<float>();
}

void setFloatMember(const Ref &value, std::size_t index, float number)
{
    value.field(value.type().fields[index]).as<float>() = number;
}

// Text shown for a value the Inspector cannot edit.
std::string summary(const Ref &value)
{
    const TypeInfo &type = value.type();
    if (type.kind == TypeKind::Sequence || type.kind == TypeKind::Map) {
        const std::size_t count = value.size();
        return std::to_string(count) + (count == 1 ? " element" : " elements");
    }
    if (type.kind == TypeKind::Optional)
        return value.hasValue() ? "(set)" : "(empty)";
    if (!type.print)
        return "(not inspectable)";
    std::ostringstream text;
    type.print(text, value.data());
    std::string result = text.str();
    constexpr std::size_t MaxLength = 64;
    if (result.size() > MaxLength)
        result = result.substr(0, MaxLength - 3) + "...";
    return result;
}

PropertyValue readValue(const Ref &value)
{
    const TypeInfo &type = value.type();
    switch (shapeOf(type)) {
    case Shape::Bool:
        return value.as<bool>();
    case Shape::Int:
        return findInteger(type)->read(value.data());
    case Shape::Float:
        return findFloat(type)->read(value.data());
    case Shape::String:
        return value.as<std::string>();
    case Shape::Enum:
        return clampTo<int>(*value.enumValue());
    case Shape::Vec2:
        return Vec2{floatMember(value, 0), floatMember(value, 1)};
    case Shape::Color:
        return Color{floatMember(value, 0), floatMember(value, 1), floatMember(value, 2), floatMember(value, 3)};
    case Shape::Text:
        return summary(value);
    case Shape::Group:
    default:
        return std::monostate{};
    }
}

// Returns false if value does not have the type the field is edited with.
bool writeValue(const Ref &target, const PropertyValue &value)
{
    const TypeInfo &type = target.type();
    switch (shapeOf(type)) {
    case Shape::Bool:
        if (const bool *flag = std::get_if<bool>(&value)) {
            target.as<bool>() = *flag;
            return true;
        }
        return false;
    case Shape::Int:
        if (const int *number = std::get_if<int>(&value)) {
            findInteger(type)->write(target.data(), *number);
            return true;
        }
        return false;
    case Shape::Float:
        if (const float *number = std::get_if<float>(&value)) {
            findFloat(type)->write(target.data(), *number);
            return true;
        }
        return false;
    case Shape::String:
        if (const std::string *text = std::get_if<std::string>(&value)) {
            target.as<std::string>() = *text;
            return true;
        }
        return false;
    case Shape::Enum:
        if (const int *number = std::get_if<int>(&value)) {
            const IntegerType *underlying = findInteger(*type.enumOps->underlying);
            return target.setEnumValue(underlying ? underlying->clamp(*number) : *number);
        }
        return false;
    case Shape::Vec2:
        if (const Vec2 *vector = std::get_if<Vec2>(&value)) {
            setFloatMember(target, 0, vector->x);
            setFloatMember(target, 1, vector->y);
            return true;
        }
        return false;
    case Shape::Color:
        if (const Color *color = std::get_if<Color>(&value)) {
            setFloatMember(target, 0, color->r);
            setFloatMember(target, 1, color->g);
            setFloatMember(target, 2, color->b);
            setFloatMember(target, 3, color->a);
            return true;
        }
        return false;
    case Shape::Group:
    case Shape::Text:
    default:
        return false;
    }
}

// Runs a change of the world; an exception (from a component constructor,
// an allocation...) is logged instead of closing the editor.
template <typename Change>
bool guarded(const std::string &what, Change &&change)
{
    try {
        return change();
    } catch (const std::exception &e) {
        Logger::instance().error("ECS", "Cannot " + what + ": " + e.what());
        return false;
    }
}

} // namespace

// ---- Ids

std::optional<ECS::Entity> EcsScene::entity(ObjectId id) const
{
    const std::uint64_t low = id & IndexMask;
    if (low == 0)
        return std::nullopt;
    const std::size_t index = static_cast<std::size_t>(low - 1);
    if (index >= _handles.size())
        return std::nullopt;
    const ECS::Entity handle = _handles[index];
    if (handle.generation() != static_cast<ECS::EntityGeneration>(id >> 32) || !_world.isAlive(handle))
        return std::nullopt;
    return handle;
}

EcsScene::ObjectId EcsScene::id(ECS::Entity entity) const
{
    if (!_world.isAlive(entity) || entity.index() >= IndexLimit)
        return InvalidId;
    // The handle given out last for an index is the most recent one: an
    // older id of that index has a lower generation, and is dead.
    if (entity.index() >= _handles.size())
        _handles.resize(entity.index() + 1);
    _handles[entity.index()] = entity;
    return (static_cast<ObjectId>(entity.generation()) << 32) | static_cast<ObjectId>(entity.index() + 1);
}

// ---- HierarchyModel

std::vector<EcsScene::ObjectId> EcsScene::roots() const
{
    const ECS::EntityList entities = _world.inspect().entities();
    std::vector<ObjectId> ids;
    ids.reserve(entities.size());
    for (const ECS::Entity entity : entities)
        if (const ObjectId objectId = id(entity); objectId != InvalidId)
            ids.push_back(objectId);
    return ids;
}

std::string EcsScene::name(ObjectId id) const
{
    const std::optional<ECS::Entity> target = entity(id);
    return target ? _world.inspect().displayName(*target) : std::string();
}

EcsScene::ObjectId EcsScene::create(ObjectId parent, const std::string &name)
{
    if (parent != InvalidId)
        return InvalidId;
    ObjectId created = InvalidId;
    guarded("create an entity", [&] {
        created = id(_world.spawnEntity(name));
        return true;
    });
    return created;
}

bool EcsScene::destroy(ObjectId id)
{
    const std::optional<ECS::Entity> target = entity(id);
    return target && guarded("delete " + name(id), [&] {
        _world.killEntity(*target);
        return true;
    });
}

bool EcsScene::rename(ObjectId id, const std::string &name)
{
    const std::optional<ECS::Entity> target = entity(id);
    return target && !name.empty() && guarded("rename an entity", [&] {
        if (_world.hasComponents<ECS::Name>(*target))
            _world.getComponent<ECS::Name>(*target).value = name;
        else
            _world.addComponent(*target, ECS::Name{name});
        return true;
    });
}

bool EcsScene::setParent(ObjectId id, ObjectId newParent)
{
    return newParent == InvalidId && exists(id);
}

std::string EcsScene::idLabel(ObjectId id) const
{
    const std::optional<ECS::Entity> target = entity(id);
    if (!target)
        return "No entity";
    return "Entity " + std::to_string(target->index()) + ", generation " + std::to_string(target->generation());
}

// ---- PropertyModel

const ComponentInfo &EcsScene::describe(const TypeInfo &type) const
{
    auto it = _descriptions.find(&type);
    if (it == _descriptions.end()) {
        ComponentInfo info;
        info.name = type.name;
        // A component that is not a struct (a registered scalar or opaque
        // type) has no field to show.
        if (type.kind == TypeKind::Struct)
            for (const reflect::FieldInfo &field : type.fields)
                info.properties.push_back(describeField(field, false));
        it = _descriptions.emplace(&type, std::move(info)).first;
    }
    return it->second;
}

std::vector<ComponentInfo> EcsScene::components(ObjectId id) const
{
    std::vector<ComponentInfo> result;
    const std::optional<ECS::Entity> target = entity(id);
    if (!target)
        return result;
    for (const Ref &component : _world.inspect().list(*target))
        if (&component.type() != nameType())
            result.push_back(describe(component.type()));
    return result;
}

std::optional<EcsScene::Field> EcsScene::resolve(ObjectId id, const std::string &component, const std::string &path) const
{
    const std::optional<ECS::Entity> target = entity(id);
    const TypeInfo *type = reflect::RegistryInspector::findType(component);
    if (!target || !type || path.empty())
        return std::nullopt;
    Field field{_world.inspect().get(*target, *type)};
    if (!field.ref)
        return std::nullopt;

    // Field names separated by dots; the editor never builds index or key steps.
    for (std::size_t start = 0; start <= path.size();) {
        const std::size_t dot = std::min(path.find('.', start), path.size());
        const reflect::FieldInfo *info =
            field.ref.is(TypeKind::Struct) ? field.ref.type().findField(std::string_view(path).substr(start, dot - start)) : nullptr;
        if (!info)
            return std::nullopt;
        field.readOnly = field.readOnly || info->readOnly;
        field.ref = field.ref.field(*info);
        start = dot + 1;
    }
    return field;
}

PropertyValue EcsScene::get(ObjectId id, const std::string &component, const std::string &path) const
{
    const std::optional<Field> field = resolve(id, component, path);
    return field ? readValue(field->ref) : PropertyValue{std::monostate{}};
}

bool EcsScene::set(ObjectId id, const std::string &component, const std::string &path, const PropertyValue &value)
{
    const std::optional<Field> field = resolve(id, component, path);
    if (!field || field->readOnly || hasConstMember(field->ref.type()))
        return false;
    return guarded("set " + component + "." + path, [&] { return writeValue(field->ref, value); });
}

std::vector<std::string> EcsScene::addableComponents(ObjectId id) const
{
    std::vector<std::string> names;
    const std::optional<ECS::Entity> target = entity(id);
    if (!target)
        return names;
    const ECS::reflect::RegistryInspector inspector = _world.inspect();
    for (const TypeInfo *type : reflect::RegistryInspector::types())
        if (type != nameType() && type->construct && !inspector.has(*target, *type))
            names.push_back(type->name);
    std::ranges::sort(names);
    return names;
}

bool EcsScene::addComponent(ObjectId id, const std::string &component)
{
    const std::optional<ECS::Entity> target = entity(id);
    const TypeInfo *type = reflect::RegistryInspector::findType(component);
    if (!target || !type || type == nameType())
        return false;
    ECS::reflect::RegistryInspector inspector = _world.inspect();
    return !inspector.has(*target, *type)
        && guarded("add " + component, [&] { return inspector.add(*target, *type).valid(); });
}

bool EcsScene::removeComponent(ObjectId id, const std::string &component)
{
    const std::optional<ECS::Entity> target = entity(id);
    const TypeInfo *type = reflect::RegistryInspector::findType(component);
    if (!target || !type || type == nameType())
        return false;
    ECS::reflect::RegistryInspector inspector = _world.inspect();
    return inspector.has(*target, *type) && guarded("remove " + component, [&] {
        inspector.remove(*target, *type);
        return true;
    });
}
