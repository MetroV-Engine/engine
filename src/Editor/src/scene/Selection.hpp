/*
    Created on: 30/03/2026
    Filename: Selection.hpp
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

// The object currently selected in the editor, shared by the panels (the
// Hierarchy sets it, the Inspector shows it). It stores an id, never a
// reference to the object's data, so it stays safe if the object is destroyed.
class Selection
{
public:
    using ObjectId = HierarchyModel::ObjectId;

    ObjectId get() const { return _selected; }
    bool has() const { return _selected != HierarchyModel::InvalidId; }
    bool is(ObjectId id) const { return id != HierarchyModel::InvalidId && _selected == id; }
    void set(ObjectId id) { _selected = id; }
    void clear() { _selected = HierarchyModel::InvalidId; }

private:
    ObjectId _selected = HierarchyModel::InvalidId;
};
