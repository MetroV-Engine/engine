/*
    Created on: 30/03/2026
    Filename: HierarchyModel.hpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#pragma once

#include <cstdint>
#include <string>
#include <vector>

// The scene objects shown by the Hierarchy panel, seen as a tree. The panel
// only talks to this interface, so the data can come from the built-in demo
// scene (DemoScene) or from the engine's ECS (EcsScene, which maps entities to
// ObjectIds; its tree is flat until the ECS has a parent component).
class HierarchyModel
{
public:
    // Opaque, stable identifier of an object (e.g. an ECS entity handle packed
    // into 64 bits). InvalidId means "none" (no parent, no selection...).
    using ObjectId = std::uint64_t;
    static constexpr ObjectId InvalidId = 0;

    virtual ~HierarchyModel() = default;

    // Objects without a parent, in display order.
    virtual std::vector<ObjectId> roots() const = 0;
    // Direct children, in display order. Empty if none or if id does not exist.
    virtual std::vector<ObjectId> children(ObjectId id) const = 0;
    virtual ObjectId parent(ObjectId id) const = 0;
    virtual bool exists(ObjectId id) const = 0;
    virtual std::string name(ObjectId id) const = 0;

    // Editing. Each returns InvalidId / false when the change is refused.
    // Creates an object under parent (InvalidId for a root).
    virtual ObjectId create(ObjectId parent, const std::string &name) = 0;
    // Destroys the object and all its descendants.
    virtual bool destroy(ObjectId id) = 0;
    virtual bool rename(ObjectId id, const std::string &name) = 0;
    // Moves the object (with its subtree) under newParent (InvalidId for the
    // root). Refused if newParent is the object itself or one of its descendants.
    virtual bool setParent(ObjectId id, ObjectId newParent) = 0;

    // False for a flat scene, whose objects cannot have children: the
    // Hierarchy then offers neither "Create Child" nor drag and drop.
    virtual bool supportsParenting() const { return true; }
    // Identity shown under the object's name in the Inspector.
    virtual std::string idLabel(ObjectId id) const { return "Id " + std::to_string(id); }

    // True if ancestor is id's parent, grandparent, etc.
    bool isAncestor(ObjectId ancestor, ObjectId id) const
    {
        for (ObjectId current = parent(id); current != InvalidId; current = parent(current))
            if (current == ancestor)
                return true;
        return false;
    }

    // Number of objects below id, at any depth.
    std::size_t countDescendants(ObjectId id) const
    {
        std::size_t count = 0;
        for (ObjectId child : children(id))
            count += 1 + countDescendants(child);
        return count;
    }
};
