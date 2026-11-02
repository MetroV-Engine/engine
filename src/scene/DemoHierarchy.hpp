/*
    Created on: 30/03/2026
    Filename: DemoHierarchy.hpp
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

#include <unordered_map>

// In-memory scene used until the engine's ECS is connected: a small
// MetroidVania level (rooms, tilemaps, enemies, collectibles).
class DemoHierarchy : public HierarchyModel
{
public:
    // Fills the demo level.
    DemoHierarchy();

    std::vector<ObjectId> roots() const override { return _roots; }
    std::vector<ObjectId> children(ObjectId id) const override;
    ObjectId parent(ObjectId id) const override;
    bool exists(ObjectId id) const override { return _objects.contains(id); }
    std::string name(ObjectId id) const override;

    ObjectId create(ObjectId parent, const std::string &name) override;
    bool destroy(ObjectId id) override;
    bool rename(ObjectId id, const std::string &name) override;
    bool setParent(ObjectId id, ObjectId newParent) override;

private:
    struct Object
    {
        std::string name;
        ObjectId parent = InvalidId;
        std::vector<ObjectId> children;
    };

    std::unordered_map<ObjectId, Object> _objects;
    std::vector<ObjectId> _roots;
    ObjectId _nextId = 1;

    // Children list of parent (the roots for InvalidId).
    std::vector<ObjectId> &siblingsOf(ObjectId parent);
};
