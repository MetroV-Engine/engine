/*
    Created on: 03/11/2026
    Filename: EcsScene.hpp
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
#include "scene/PropertyModel.hpp"

#include "ECS/ECS.hpp"
#include "ECS/Reflection/Reflection.hpp"

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

// The entities of an ECS world, as the Hierarchy and the Inspector see them.
// Everything goes through world.inspect() and Ref: the editor needs no
// component header.
//
// Hierarchy: every live entity is a root object (the ECS has no parent
// component yet, so the tree is flat), labelled by its ECS::Name or
// "Entity <index>". Creating an object spawns a named entity; deleting it
// kills the entity.
//
// Inspector: the entity's components whose type is reflected (E_REFLECT, or
// reflect::registerComponentType). Their fields become properties:
//   bool                          -> Bool
//   integers                      -> Int, clamped to the field's range
//   float, double                 -> Float
//   std::string                   -> String
//   enum                          -> Int, its raw value (names are not reflected)
//   struct { float x, y; }        -> Vec2
//   struct { float r, g, b, a; }  -> Color
//   any other reflected struct    -> Group of its fields
//   vector, map, optional, other  -> read-only summary text
// Const members are read-only. ECS::Name is not listed as a component: it is
// the object's name, edited above the components.
//
// An ObjectId packs the entity's index and generation, so the id of a killed
// entity never designates a later entity reusing its index.
class EcsScene : public HierarchyModel, public PropertyModel
{
public:
    using ObjectId = HierarchyModel::ObjectId;

    // world must outlive the scene.
    explicit EcsScene(ECS::Registry &world) : _world(world) {}

    ECS::Registry &world() const { return _world; }

    // The live entity designated by id; nullopt if the id is unknown or the
    // entity was killed since.
    std::optional<ECS::Entity> entity(ObjectId id) const;
    // The id of a live entity of this world (InvalidId otherwise).
    ObjectId id(ECS::Entity entity) const;

    // HierarchyModel
    std::vector<ObjectId> roots() const override;
    std::vector<ObjectId> children(ObjectId) const override { return {}; }
    ObjectId parent(ObjectId) const override { return InvalidId; }
    bool exists(ObjectId id) const override { return entity(id).has_value(); }
    std::string name(ObjectId id) const override;
    // Spawns an entity named name. Refused under a parent: the tree is flat.
    ObjectId create(ObjectId parent, const std::string &name) override;
    bool destroy(ObjectId id) override;
    // Sets the entity's ECS::Name, adding it if missing.
    bool rename(ObjectId id, const std::string &name) override;
    // Only "move to the root" is accepted, where every entity already is.
    bool setParent(ObjectId id, ObjectId newParent) override;
    bool supportsParenting() const override { return false; }
    std::string idLabel(ObjectId id) const override;

    // PropertyModel
    std::vector<ComponentInfo> components(ObjectId id) const override;
    PropertyValue get(ObjectId id, const std::string &component, const std::string &path) const override;
    bool set(ObjectId id, const std::string &component, const std::string &path, const PropertyValue &value) override;
    // Sorted by name; types without a default constructor cannot be added.
    std::vector<std::string> addableComponents(ObjectId id) const override;
    bool addComponent(ObjectId id, const std::string &component) override;
    bool removeComponent(ObjectId id, const std::string &component) override;

private:
    // A field of a component, and whether it is (inside) a const member.
    struct Field
    {
        ECS::reflect::Ref ref;
        bool readOnly = false;
    };

    ECS::Registry &_world;
    // The handle behind each id given out, by entity index: an Entity cannot
    // be rebuilt from its index and generation, so ids are resolved here.
    mutable std::vector<ECS::Entity> _handles;
    // Properties of each component type, built on first use.
    mutable std::unordered_map<const ECS::reflect::TypeInfo *, ComponentInfo> _descriptions;

    // The field at path ("speed", "movement.speed") of the entity's component.
    std::optional<Field> resolve(ObjectId id, const std::string &component, const std::string &path) const;
    const ComponentInfo &describe(const ECS::reflect::TypeInfo &type) const;
};
