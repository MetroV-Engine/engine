/*
    Created on: 30/03/2026
    Filename: DemoScene.hpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#pragma once

#include "HierarchyModel.hpp"
#include "PropertyModel.hpp"

#include <unordered_map>

// In-memory scene used until the engine's ECS is connected: a small
// MetroidVania level (rooms, tilemaps, enemies, collectibles) whose objects
// carry components (Transform, Sprite, RigidBody, Health...).
class DemoScene : public HierarchyModel, public PropertyModel
{
public:
    using ObjectId = HierarchyModel::ObjectId;

    // Fills the demo level.
    DemoScene();

    // HierarchyModel
    std::vector<ObjectId> roots() const override { return _roots; }
    std::vector<ObjectId> children(ObjectId id) const override;
    ObjectId parent(ObjectId id) const override;
    bool exists(ObjectId id) const override { return _objects.contains(id); }
    std::string name(ObjectId id) const override;
    // New objects get a Transform.
    ObjectId create(ObjectId parent, const std::string &name) override;
    bool destroy(ObjectId id) override;
    bool rename(ObjectId id, const std::string &name) override;
    bool setParent(ObjectId id, ObjectId newParent) override;

    // PropertyModel
    std::vector<ComponentInfo> components(ObjectId id) const override;
    PropertyValue get(ObjectId id, const std::string &component, const std::string &path) const override;
    bool set(ObjectId id, const std::string &component, const std::string &path, const PropertyValue &value) override;
    std::vector<std::string> addableComponents(ObjectId id) const override;
    bool addComponent(ObjectId id, const std::string &component) override;
    bool removeComponent(ObjectId id, const std::string &component) override;

private:
    struct Component
    {
        std::string type;
        std::unordered_map<std::string, PropertyValue> values; // by path
    };
    struct Object
    {
        std::string name;
        ObjectId parent = InvalidId;
        std::vector<ObjectId> children;
        std::vector<Component> components;
    };

    std::unordered_map<ObjectId, Object> _objects;
    std::vector<ObjectId> _roots;
    ObjectId _nextId = 1;

    // Children list of parent (the roots for InvalidId).
    std::vector<ObjectId> &siblingsOf(ObjectId parent);
    Component *findComponent(ObjectId id, const std::string &type);
    const Component *findComponent(ObjectId id, const std::string &type) const;
    // Demo setup helper: adds a component and overrides some of its defaults.
    void attach(ObjectId id, const std::string &type, const std::vector<std::pair<std::string, PropertyValue>> &values = {});
};
