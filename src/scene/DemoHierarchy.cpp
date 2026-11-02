/*
    Created on: 30/03/2026
    Filename: DemoHierarchy.cpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "DemoHierarchy.hpp"

#include <algorithm>
#include <string>

DemoHierarchy::DemoHierarchy()
{
    create(InvalidId, "Main Camera");
    const ObjectId player = create(InvalidId, "Player");
    create(player, "Sprite");
    create(player, "Hitbox");
    create(player, "Weapon: Whip");

    const ObjectId level = create(InvalidId, "Level 01 - Forgotten Caverns");
    const ObjectId entrance = create(level, "Room: Entrance Hall");
    create(entrance, "Tilemap");
    create(entrance, "Door -> Crystal Shaft");
    create(entrance, "Save Point");
    const ObjectId shaft = create(level, "Room: Crystal Shaft");
    create(shaft, "Tilemap");
    const ObjectId enemies = create(shaft, "Enemies");
    for (int i = 1; i <= 4; i++)
        create(enemies, "Crawler " + std::to_string(i));
    create(enemies, "Bat Swarm");
    create(shaft, "Double Jump Upgrade");
    const ObjectId boss = create(level, "Room: Guardian Lair");
    create(boss, "Tilemap");
    create(boss, "Boss: Stone Guardian");

    // A large group, to show that long lists stay fast.
    const ObjectId collectibles = create(level, "Collectibles");
    for (int i = 1; i <= 250; i++)
        create(collectibles, "Soul Shard " + std::to_string(i));

    create(InvalidId, "Background Music");
}

std::vector<HierarchyModel::ObjectId> DemoHierarchy::children(ObjectId id) const
{
    const auto it = _objects.find(id);
    return it != _objects.end() ? it->second.children : std::vector<ObjectId>{};
}

HierarchyModel::ObjectId DemoHierarchy::parent(ObjectId id) const
{
    const auto it = _objects.find(id);
    return it != _objects.end() ? it->second.parent : InvalidId;
}

std::string DemoHierarchy::name(ObjectId id) const
{
    const auto it = _objects.find(id);
    return it != _objects.end() ? it->second.name : std::string();
}

std::vector<HierarchyModel::ObjectId> &DemoHierarchy::siblingsOf(ObjectId parent)
{
    return parent == InvalidId ? _roots : _objects.at(parent).children;
}

HierarchyModel::ObjectId DemoHierarchy::create(ObjectId parent, const std::string &name)
{
    if (parent != InvalidId && !exists(parent))
        return InvalidId;
    const ObjectId id = _nextId++;
    _objects[id] = {name, parent, {}};
    siblingsOf(parent).push_back(id);
    return id;
}

bool DemoHierarchy::destroy(ObjectId id)
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

bool DemoHierarchy::rename(ObjectId id, const std::string &name)
{
    if (!exists(id) || name.empty())
        return false;
    _objects.at(id).name = name;
    return true;
}

bool DemoHierarchy::setParent(ObjectId id, ObjectId newParent)
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
