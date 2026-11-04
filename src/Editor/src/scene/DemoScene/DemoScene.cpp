/*
    Created on: 30/03/2026
    Filename: DemoScene.cpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "DemoScene.hpp"

#include <algorithm>
#include <string>
#include <utility>

namespace {

// A component type of the demo: its description and default values.
struct ComponentType
{
    ComponentInfo info;
    std::vector<std::pair<std::string, PropertyValue>> defaults;
};

PropertyInfo prop(std::string name, PropertyType type, float min = 0.0f, float max = 0.0f, float speed = 0.1f,
    std::string format = "")
{
    PropertyInfo info;
    info.name = std::move(name);
    info.type = type;
    info.min = min;
    info.max = max;
    info.speed = speed;
    info.format = std::move(format);
    return info;
}

PropertyInfo enumProp(std::string name, std::vector<std::string> options)
{
    PropertyInfo info = prop(std::move(name), PropertyType::Enum);
    info.enumOptions = std::move(options);
    return info;
}

PropertyInfo group(std::string name, std::vector<PropertyInfo> fields)
{
    PropertyInfo info = prop(std::move(name), PropertyType::Group);
    info.fields = std::move(fields);
    return info;
}

PropertyInfo readOnly(PropertyInfo info, std::string tooltip)
{
    info.readOnly = true;
    info.tooltip = std::move(tooltip);
    return info;
}

using T = PropertyType;

const std::vector<ComponentType> &componentTypes()
{
    static const std::vector<ComponentType> types = {
        {{"Transform",
             {prop("position", T::Vec2, 0, 0, 0.5f, "%.1f"), prop("rotation", T::Float, -360, 360, 0.5f, "%.1f deg"),
                 prop("scale", T::Vec2, 0, 0, 0.01f, "%.2f")},
             false},
            {{"position", Vec2{}}, {"rotation", 0.0f}, {"scale", Vec2{1, 1}}}},
        {{"Sprite",
             {prop("texture", T::String), prop("tint", T::Color), prop("layer", T::Int, -100, 100, 0.2f),
                 prop("flipX", T::Bool), prop("visible", T::Bool)}},
            {{"texture", std::string("")}, {"tint", Color{}}, {"layer", 0}, {"flipX", false}, {"visible", true}}},
        {{"RigidBody",
             {enumProp("bodyType", {"Static", "Dynamic", "Kinematic"}),
                 prop("mass", T::Float, 0.01f, 1000, 0.05f, "%.2f kg"), prop("gravityScale", T::Float, 0, 10, 0.01f, "%.2f"),
                 prop("fixedRotation", T::Bool)}},
            {{"bodyType", 1}, {"mass", 1.0f}, {"gravityScale", 1.0f}, {"fixedRotation", true}}},
        {{"BoxCollider",
             {prop("size", T::Vec2, 0, 10000, 0.5f, "%.1f"), prop("offset", T::Vec2, 0, 0, 0.5f, "%.1f"),
                 prop("isTrigger", T::Bool)}},
            {{"size", Vec2{16, 16}}, {"offset", Vec2{}}, {"isTrigger", false}}},
        {{"Health", {prop("max", T::Int, 1, 9999, 1), prop("current", T::Int, 0, 9999, 1), prop("invulnerable", T::Bool)}},
            {{"max", 100}, {"current", 100}, {"invulnerable", false}}},
        {{"PlayerController",
             {group("movement",
                  {prop("speed", T::Float, 0, 1000, 1, "%.0f px/s"), prop("acceleration", T::Float, 0, 10000, 5, "%.0f px/s2")}),
                 group("jump",
                     {prop("force", T::Float, 0, 2000, 1, "%.0f"), prop("maxJumps", T::Int, 1, 3, 0.05f),
                         prop("coyoteTime", T::Float, 0, 0.5f, 0.005f, "%.2f s")})}},
            {{"movement.speed", 140.0f}, {"movement.acceleration", 900.0f}, {"jump.force", 420.0f}, {"jump.maxJumps", 1},
                {"jump.coyoteTime", 0.1f}}},
        {{"EnemyAI",
             {enumProp("behavior", {"Idle", "Patrol", "Chase", "Flee"}),
                 prop("detectionRange", T::Float, 0, 1000, 1, "%.0f px"), prop("damage", T::Int, 0, 999, 0.2f),
                 prop("target", T::String)}},
            {{"behavior", 1}, {"detectionRange", 96.0f}, {"damage", 10}, {"target", std::string("Player")}}},
        {{"Camera",
             {prop("zoom", T::Float, 0.1f, 10, 0.01f, "%.2fx"), prop("followTarget", T::String),
                 prop("deadZone", T::Vec2, 0, 1000, 0.5f, "%.0f"), prop("pixelPerfect", T::Bool)}},
            {{"zoom", 2.0f}, {"followTarget", std::string("Player")}, {"deadZone", Vec2{32, 24}}, {"pixelPerfect", true}}},
        {{"Tilemap",
             {prop("tileset", T::String), prop("tileSize", T::Int, 1, 256, 0.2f), prop("width", T::Int, 1, 1024, 0.2f),
                 prop("height", T::Int, 1, 1024, 0.2f), readOnly(prop("tileCount", T::Int), "Computed: width x height")}},
            {{"tileset", std::string("tiles/caverns.png")}, {"tileSize", 16}, {"width", 40}, {"height", 23},
                {"tileCount", 920}}},
        {{"AudioSource",
             {prop("clip", T::String), prop("volume", T::Float, 0, 1, 0.01f, "%.2f"), prop("loop", T::Bool),
                 prop("playOnStart", T::Bool)}},
            {{"clip", std::string("")}, {"volume", 0.8f}, {"loop", false}, {"playOnStart", false}}},
        {{"Collectible", {enumProp("kind", {"Soul Shard", "Health Orb", "Upgrade"}), prop("value", T::Int, 0, 9999, 0.2f)}},
            {{"kind", 0}, {"value", 1}}},
        {{"Door", {prop("targetRoom", T::String), prop("locked", T::Bool), prop("requiredKey", T::String)}},
            {{"targetRoom", std::string("")}, {"locked", false}, {"requiredKey", std::string("")}}},
    };
    return types;
}

const ComponentType *findType(const std::string &name)
{
    for (const ComponentType &type : componentTypes())
        if (type.info.name == name)
            return &type;
    return nullptr;
}

// Finds a property description from a path like "jump.force".
const PropertyInfo *findProperty(const std::vector<PropertyInfo> &properties, const std::string &path)
{
    const std::size_t dot = path.find('.');
    const std::string head = path.substr(0, dot);
    for (const PropertyInfo &property : properties) {
        if (property.name != head)
            continue;
        if (dot == std::string::npos)
            return property.type == PropertyType::Group ? nullptr : &property;
        return property.type == PropertyType::Group ? findProperty(property.fields, path.substr(dot + 1)) : nullptr;
    }
    return nullptr;
}

// Whether value holds the C++ type used by properties of this type.
bool matchesType(const PropertyValue &value, PropertyType type)
{
    switch (type) {
    case PropertyType::Bool:
        return std::holds_alternative<bool>(value);
    case PropertyType::Int:
    case PropertyType::Enum:
        return std::holds_alternative<int>(value);
    case PropertyType::Float:
        return std::holds_alternative<float>(value);
    case PropertyType::String:
        return std::holds_alternative<std::string>(value);
    case PropertyType::Vec2:
        return std::holds_alternative<Vec2>(value);
    case PropertyType::Color:
        return std::holds_alternative<Color>(value);
    case PropertyType::Group:
    default:
        return false;
    }
}

// Applies the property's bounds (and the enum range).
PropertyValue clampValue(PropertyValue value, const PropertyInfo &info)
{
    const bool bounded = info.min != info.max;
    if (int *i = std::get_if<int>(&value)) {
        if (info.type == PropertyType::Enum)
            *i = std::clamp(*i, 0, std::max(0, static_cast<int>(info.enumOptions.size()) - 1));
        else if (bounded)
            *i = std::clamp(*i, static_cast<int>(info.min), static_cast<int>(info.max));
    } else if (float *f = std::get_if<float>(&value); f && bounded) {
        *f = std::clamp(*f, info.min, info.max);
    } else if (Vec2 *v = std::get_if<Vec2>(&value); v && bounded) {
        v->x = std::clamp(v->x, info.min, info.max);
        v->y = std::clamp(v->y, info.min, info.max);
    }
    return value;
}

} // namespace

DemoScene::DemoScene(bool withDemoLevel)
{
    if (withDemoLevel)
        loadDemoLevel();
}

void DemoScene::loadDemoLevel()
{
    const ObjectId camera = create(InvalidId, "Main Camera");
    attach(camera, "Camera");

    const ObjectId player = create(InvalidId, "Player");
    attach(player, "Transform", {{"position", Vec2{64, 160}}});
    attach(player, "Sprite", {{"texture", std::string("sprites/hero_idle.png")}, {"layer", 10}});
    attach(player, "RigidBody");
    attach(player, "BoxCollider", {{"size", Vec2{12, 28}}});
    attach(player, "Health", {{"max", 5}, {"current", 5}});
    attach(player, "PlayerController");
    attach(create(player, "Sprite"), "Sprite", {{"texture", std::string("sprites/hero_cape.png")}});
    attach(create(player, "Hitbox"), "BoxCollider", {{"size", Vec2{12, 28}}, {"isTrigger", true}});
    attach(create(player, "Weapon: Whip"), "BoxCollider", {{"size", Vec2{48, 8}}, {"offset", Vec2{30, 0}}, {"isTrigger", true}});

    const ObjectId level = create(InvalidId, "Level 01 - Forgotten Caverns");
    const ObjectId entrance = create(level, "Room: Entrance Hall");
    attach(create(entrance, "Tilemap"), "Tilemap");
    const ObjectId door = create(entrance, "Door -> Crystal Shaft");
    attach(door, "Door", {{"targetRoom", std::string("Crystal Shaft")}});
    attach(door, "BoxCollider", {{"size", Vec2{16, 48}}, {"isTrigger", true}});
    attach(create(entrance, "Save Point"), "Sprite", {{"texture", std::string("sprites/save_statue.png")}});

    const ObjectId shaft = create(level, "Room: Crystal Shaft");
    attach(create(shaft, "Tilemap"), "Tilemap", {{"width", 20}, {"height", 60}});
    const ObjectId enemies = create(shaft, "Enemies");
    for (int i = 1; i <= 4; i++) {
        const ObjectId crawler = create(enemies, "Crawler " + std::to_string(i));
        attach(crawler, "Transform", {{"position", Vec2{40.0f * i, 300.0f + 80.0f * i}}});
        attach(crawler, "Sprite", {{"texture", std::string("sprites/crawler.png")}});
        attach(crawler, "RigidBody");
        attach(crawler, "BoxCollider", {{"size", Vec2{16, 10}}});
        attach(crawler, "Health", {{"max", 20}, {"current", 20}});
        attach(crawler, "EnemyAI");
    }
    const ObjectId bats = create(enemies, "Bat Swarm");
    attach(bats, "EnemyAI", {{"behavior", 2}, {"detectionRange", 160.0f}, {"damage", 5}});
    attach(create(shaft, "Double Jump Upgrade"), "Collectible", {{"kind", 2}});

    const ObjectId boss = create(level, "Room: Guardian Lair");
    attach(create(boss, "Tilemap"), "Tilemap");
    const ObjectId guardian = create(boss, "Boss: Stone Guardian");
    attach(guardian, "Sprite", {{"texture", std::string("sprites/stone_guardian.png")}});
    attach(guardian, "Health", {{"max", 600}, {"current", 600}});
    attach(guardian, "EnemyAI", {{"behavior", 0}, {"damage", 25}});

    // A large group, to show that long lists stay fast.
    const ObjectId collectibles = create(level, "Collectibles");
    for (int i = 1; i <= 250; i++) {
        const ObjectId shard = create(collectibles, "Soul Shard " + std::to_string(i));
        attach(shard, "Transform", {{"position", Vec2{16.0f * (i % 40), 32.0f * (i / 40)}}});
        attach(shard, "Collectible");
    }

    const ObjectId music = create(InvalidId, "Background Music");
    attach(music, "AudioSource", {{"clip", std::string("music/caverns.ogg")}, {"loop", true}, {"playOnStart", true}});
}

// ---- HierarchyModel

std::vector<DemoScene::ObjectId> DemoScene::children(ObjectId id) const
{
    const auto it = _objects.find(id);
    return it != _objects.end() ? it->second.children : std::vector<ObjectId>{};
}

DemoScene::ObjectId DemoScene::parent(ObjectId id) const
{
    const auto it = _objects.find(id);
    return it != _objects.end() ? it->second.parent : InvalidId;
}

std::string DemoScene::name(ObjectId id) const
{
    const auto it = _objects.find(id);
    return it != _objects.end() ? it->second.name : std::string();
}

std::vector<DemoScene::ObjectId> &DemoScene::siblingsOf(ObjectId parent)
{
    return parent == InvalidId ? _roots : _objects.at(parent).children;
}

DemoScene::ObjectId DemoScene::create(ObjectId parent, const std::string &name)
{
    if (parent != InvalidId && !exists(parent))
        return InvalidId;
    const ObjectId id = _nextId++;
    _objects[id] = {name, parent, {}, {}};
    siblingsOf(parent).push_back(id);
    addComponent(id, "Transform");
    return id;
}

bool DemoScene::destroy(ObjectId id)
{
    if (!exists(id))
        return false;
    // Copy: destroying a child edits this object's children list.
    for (ObjectId child : std::vector<ObjectId>(_objects.at(id).children))
        destroy(child);
    std::erase(siblingsOf(_objects.at(id).parent), id);
    _objects.erase(id);
    return true;
}

bool DemoScene::rename(ObjectId id, const std::string &name)
{
    if (!exists(id) || name.empty())
        return false;
    _objects.at(id).name = name;
    return true;
}

bool DemoScene::setParent(ObjectId id, ObjectId newParent)
{
    if (!exists(id) || (newParent != InvalidId && !exists(newParent)))
        return false;
    if (newParent == id || (newParent != InvalidId && isAncestor(id, newParent)))
        return false;
    Object &object = _objects.at(id);
    if (object.parent == newParent)
        return true;
    std::erase(siblingsOf(object.parent), id);
    object.parent = newParent;
    siblingsOf(newParent).push_back(id);
    return true;
}

// ---- PropertyModel

DemoScene::Component *DemoScene::findComponent(ObjectId id, const std::string &type)
{
    const auto it = _objects.find(id);
    if (it == _objects.end())
        return nullptr;
    for (Component &component : it->second.components)
        if (component.type == type)
            return &component;
    return nullptr;
}

const DemoScene::Component *DemoScene::findComponent(ObjectId id, const std::string &type) const
{
    return const_cast<DemoScene *>(this)->findComponent(id, type);
}

std::vector<ComponentInfo> DemoScene::components(ObjectId id) const
{
    std::vector<ComponentInfo> result;
    const auto it = _objects.find(id);
    if (it == _objects.end())
        return result;
    for (const Component &component : it->second.components)
        if (const ComponentType *type = findType(component.type))
            result.push_back(type->info);
    return result;
}

PropertyValue DemoScene::get(ObjectId id, const std::string &component, const std::string &path) const
{
    const Component *data = findComponent(id, component);
    if (!data)
        return std::monostate{};
    const auto it = data->values.find(path);
    return it != data->values.end() ? it->second : PropertyValue{std::monostate{}};
}

bool DemoScene::set(ObjectId id, const std::string &component, const std::string &path, const PropertyValue &value)
{
    Component *data = findComponent(id, component);
    const ComponentType *type = findType(component);
    if (!data || !type)
        return false;
    const PropertyInfo *info = findProperty(type->info.properties, path);
    if (!info || info->readOnly || !matchesType(value, info->type))
        return false;
    data->values[path] = clampValue(value, *info);

    // Derived values of the demo components.
    if (component == "Tilemap" && (path == "width" || path == "height"))
        data->values["tileCount"] = std::get<int>(data->values["width"]) * std::get<int>(data->values["height"]);
    if (component == "Health") {
        int &current = std::get<int>(data->values["current"]);
        current = std::min(current, std::get<int>(data->values["max"]));
    }
    return true;
}

std::vector<std::string> DemoScene::addableComponents(ObjectId id) const
{
    std::vector<std::string> result;
    if (!exists(id))
        return result;
    for (const ComponentType &type : componentTypes())
        if (!findComponent(id, type.info.name))
            result.push_back(type.info.name);
    return result;
}

bool DemoScene::addComponent(ObjectId id, const std::string &component)
{
    const ComponentType *type = findType(component);
    if (!exists(id) || !type || findComponent(id, component))
        return false;
    Component data{component, {}};
    for (const auto &[path, value] : type->defaults)
        data.values[path] = value;
    _objects.at(id).components.push_back(std::move(data));
    return true;
}

bool DemoScene::removeComponent(ObjectId id, const std::string &component)
{
    const ComponentType *type = findType(component);
    if (!exists(id) || !type || !type->info.removable)
        return false;
    return std::erase_if(_objects.at(id).components, [&](const Component &c) { return c.type == component; }) > 0;
}

void DemoScene::attach(ObjectId id, const std::string &type, const std::vector<std::pair<std::string, PropertyValue>> &values)
{
    addComponent(id, type);
    for (const auto &[path, value] : values)
        set(id, type, path, value);
}
