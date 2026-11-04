/*
    Created on: 30/03/2026
    Filename: HierarchyPanel.hpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#pragma once

#include "panels/Panel/Panel.hpp"
#include "ui/TextInput/TextInput.hpp"
#include "ui/DialogManager/DialogManager.hpp"
#include "ui/NotificationManager/NotificationManager.hpp"
#include "scene/HierarchyModel.hpp"
#include "scene/Selection.hpp"

#include <functional>
#include <string>
#include <unordered_set>
#include <vector>

// Tree of the scene objects (from a HierarchyModel), to browse and organise
// the level:
//  - click to select (shared with the Inspector), arrows to move/expand;
//  - search box: shows matching objects with their parents;
//  - right click: create child, rename (F2), delete (Del, with confirmation);
//  - drag an object onto another to make it its child, or onto the empty area
//    to move it to the root (unless the model is flat, see supportsParenting).
// Changes are applied after the tree has been drawn, never while iterating it.
class HierarchyPanel : public Panel
{
public:
    using ObjectId = HierarchyModel::ObjectId;

    HierarchyPanel(HierarchyModel &model, Selection &selection, NotificationManager &notifications,
        DialogManager &dialogs);

    // Creates an object under parent (InvalidId for the root), selects it and
    // starts renaming it.
    void createObject(ObjectId parent);
    // Asks for confirmation, then deletes the object and its children.
    void requestDelete(ObjectId id);
    void startRename(ObjectId id);

protected:
    void drawContent() override;

private:
    struct Row
    {
        ObjectId id;
        int depth;
        bool hasChildren;
    };

    HierarchyModel &_model;
    Selection &_selection;
    NotificationManager &_notifications;
    DialogManager &_dialogs;
    TextInput _search;

    std::unordered_set<ObjectId> _expanded;
    ObjectId _renaming = HierarchyModel::InvalidId;
    std::string _renameBuffer;
    bool _focusRename = false;
    ObjectId _scrollTo = HierarchyModel::InvalidId;
    // Model changes requested while drawing, applied at the end of the frame.
    std::vector<std::function<void()>> _pending;

    // Flattens the visible part of the tree (expanded nodes, or search
    // results with their ancestors) into rows, so long lists can be clipped.
    std::vector<Row> buildRows(const std::string &filter) const;
    bool appendFiltered(ObjectId id, int depth, const std::string &filter, std::vector<Row> &rows) const;
    void appendExpanded(ObjectId id, int depth, std::vector<Row> &rows) const;

    void drawRow(const Row &row, bool filtering);
    void drawRenameField(const Row &row);
    void drawContextMenu(ObjectId id);
    void handleKeyboard(const std::vector<Row> &rows);
    void select(ObjectId id);
    void moveObject(ObjectId id, ObjectId newParent);
    // Expands every ancestor of id so it is visible.
    void reveal(ObjectId id);
};
