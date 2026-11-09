/*
    Created on: 30/03/2026
    Filename: HierarchyPanel.cpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "HierarchyPanel.hpp"
#include "panels/EntityDropTarget/EntityDropTarget.hpp"
#include "ui/Button/Button.hpp"

#include <imgui_stdlib.h>

#include <algorithm>
#include <cctype>
#include <cfloat>
#include <cstdint>

namespace {

constexpr const char *DragPayloadType = "HIERARCHY_OBJECT";

std::string toLower(std::string text)
{
    std::ranges::transform(text, text.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return text;
}

const void *imguiId(HierarchyModel::ObjectId id)
{
    return reinterpret_cast<const void *>(static_cast<std::uintptr_t>(id));
}

} // namespace

HierarchyPanel::HierarchyPanel(HierarchyModel &model, Selection &selection, NotificationManager &notifications,
    DialogManager &dialogs, const EntityDropTarget &entityDropTarget)
    : Panel("Hierarchy"),
      _model(model),
      _selection(selection),
      _notifications(notifications),
      _dialogs(dialogs),
      _entityDropTarget(entityDropTarget),
      _search("##HierarchySearch", "Search objects")
{
    // Top-level objects start expanded.
    for (ObjectId root : _model.roots())
        _expanded.insert(root);
}

void HierarchyPanel::createObject(ObjectId parent)
{
    _pending.emplace_back([this, parent] {
        const ObjectId id = _model.create(parent, "New Object");
        if (id == HierarchyModel::InvalidId)
            return;
        showObject(id);
        startRename(id);
    });
}

void HierarchyPanel::showObject(ObjectId id)
{
    _search.setValue("");
    reveal(id);
    select(id);
}

void HierarchyPanel::requestDelete(ObjectId id)
{
    if (!_model.exists(id))
        return;
    const std::string name = _model.name(id);
    const std::size_t descendants = _model.countDescendants(id);
    std::string message = "Delete \"" + name + "\"";
    if (descendants > 0)
        message += " and its " + std::to_string(descendants) + (descendants == 1 ? " child" : " children");
    message += "?";

    _dialogs.confirm({.title = "Delete object",
        .message = message,
        .confirmLabel = "Delete",
        .destructive = true,
        // Runs when the dialog is answered, after the panels: safe to change the model.
        .onConfirm = [this, id, name] {
            if (!_model.destroy(id))
                return;
            if (_selection.has() && !_model.exists(_selection.get()))
                _selection.clear();
            _notifications.info("Deleted", name);
        }});
}

void HierarchyPanel::startRename(ObjectId id)
{
    if (!_model.exists(id))
        return;
    _renaming = id;
    _renameBuffer = _model.name(id);
    _focusRename = true;
    _scrollTo = id;
}

void HierarchyPanel::select(ObjectId id)
{
    _selection.set(id);
}

void HierarchyPanel::reveal(ObjectId id)
{
    for (ObjectId parent = _model.parent(id); parent != HierarchyModel::InvalidId; parent = _model.parent(parent))
        _expanded.insert(parent);
    _scrollTo = id;
}

void HierarchyPanel::moveObject(ObjectId id, ObjectId newParent)
{
    if (!_model.exists(id) || _model.parent(id) == newParent)
        return;
    if (!_model.setParent(id, newParent)) {
        _notifications.warning("Cannot move \"" + _model.name(id) + "\"",
            "An object cannot be moved into itself or one of its children.");
        return;
    }
    reveal(id);
}

std::vector<HierarchyPanel::Row> HierarchyPanel::buildRows(const std::string &filter) const
{
    std::vector<Row> rows;
    for (ObjectId root : _model.roots()) {
        if (filter.empty())
            appendExpanded(root, 0, rows);
        else
            appendFiltered(root, 0, filter, rows);
    }
    return rows;
}

void HierarchyPanel::appendExpanded(ObjectId id, int depth, std::vector<Row> &rows) const
{
    const std::vector<ObjectId> children = _model.children(id);
    rows.push_back({id, depth, !children.empty()});
    if (_expanded.contains(id))
        for (ObjectId child : children)
            appendExpanded(child, depth + 1, rows);
}

bool HierarchyPanel::appendFiltered(ObjectId id, int depth, const std::string &filter, std::vector<Row> &rows) const
{
    // The row is added first (parents above children), then removed if
    // neither the object nor any descendant matches.
    const std::size_t position = rows.size();
    const std::vector<ObjectId> children = _model.children(id);
    rows.push_back({id, depth, !children.empty()});

    bool anyChildMatches = false;
    for (ObjectId child : children)
        anyChildMatches |= appendFiltered(child, depth + 1, filter, rows);
    if (anyChildMatches || toLower(_model.name(id)).find(filter) != std::string::npos)
        return true;
    rows.resize(position);
    return false;
}

void HierarchyPanel::drawContent()
{
    // The selected object may have been destroyed elsewhere.
    if (_selection.has() && !_model.exists(_selection.get()))
        _selection.clear();

    const float buttonWidth = ImGui::GetFrameHeight();
    _search.setWidth(-(buttonWidth + ImGui::GetStyle().ItemSpacing.x));
    _search.render();
    ImGui::SameLine();
    if (Button("+", {}, Button::Variant::Secondary, ImVec2(buttonWidth, 0.0f)).render())
        createObject(HierarchyModel::InvalidId);
    ImGui::SetItemTooltip("Create an object at the root");
    ImGui::Separator();

    const std::string filter = toLower(_search.getValue());
    const bool filtering = !filter.empty();
    const std::vector<Row> rows = buildRows(filter);

    // The tree handles its own keys (arrows, F2, Delete): ImGui's keyboard
    // navigation is disabled inside it so both do not move at once.
    ImGui::BeginChild("##HierarchyTree", ImVec2(0.0f, 0.0f), ImGuiChildFlags_None, ImGuiWindowFlags_NoNavInputs);
    handleKeyboard(rows);
    if (rows.empty())
        ImGui::TextDisabled(filtering ? "No object matches the search." : "The scene is empty.");

    // Only the rows on screen are drawn, so thousands of objects stay fast.
    ImGuiListClipper clipper;
    clipper.Begin(static_cast<int>(rows.size()));
    if (_scrollTo != HierarchyModel::InvalidId) {
        const auto it = std::ranges::find(rows, _scrollTo, &Row::id);
        if (it != rows.end())
            clipper.IncludeItemByIndex(static_cast<int>(it - rows.begin()));
    }
    while (clipper.Step())
        for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
            drawRow(rows[i], filtering);

    // The empty space below the tree: click to deselect, drop to move to the
    // root, right click to create an object.
    const ImVec2 available = ImGui::GetContentRegionAvail();
    ImGui::InvisibleButton("##HierarchyEmpty",
        ImVec2(std::max(available.x, 1.0f), std::max(available.y, ImGui::GetFrameHeight() * 2.0f)));
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
        _selection.clear();
    if (_model.supportsParenting() && ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(DragPayloadType)) {
            const ObjectId dragged = *static_cast<const ObjectId *>(payload->Data);
            _pending.emplace_back([this, dragged] { moveObject(dragged, HierarchyModel::InvalidId); });
        }
        ImGui::EndDragDropTarget();
    }
    if (ImGui::BeginPopupContextItem("##HierarchyEmptyMenu")) {
        if (ImGui::MenuItem("Create Object"))
            createObject(HierarchyModel::InvalidId);
        ImGui::EndPopup();
    }
    ImGui::EndChild();
    // On the child window as a whole, so the rows accept templates too.
    _entityDropTarget.accept();

    // Apply the changes requested during this frame, now that the tree is drawn.
    std::vector<std::function<void()>> pending;
    pending.swap(_pending);
    for (const auto &change : pending)
        change();
}

void HierarchyPanel::drawRow(const Row &row, bool filtering)
{
    ImGui::PushID(imguiId(row.id));
    const float indent = ImGui::GetStyle().IndentSpacing * static_cast<float>(row.depth);
    if (indent > 0.0f)
        ImGui::Indent(indent);

    if (_renaming == row.id) {
        drawRenameField(row);
    } else {
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick
            | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        if (!row.hasChildren)
            flags |= ImGuiTreeNodeFlags_Leaf;
        if (_selection.is(row.id))
            flags |= ImGuiTreeNodeFlags_Selected;

        // Search results are always shown expanded; otherwise the panel owns
        // the expanded state, since rows are built before ImGui draws them.
        ImGui::SetNextItemOpen(filtering || _expanded.contains(row.id), ImGuiCond_Always);
        const std::string name = _model.name(row.id);
        const bool open = ImGui::TreeNodeEx("##Node", flags, "%s", name.c_str());
        if (!filtering && row.hasChildren && ImGui::IsItemToggledOpen()) {
            if (open)
                _expanded.insert(row.id);
            else
                _expanded.erase(row.id);
        }
        if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && !ImGui::IsItemToggledOpen())
            select(row.id);
        if (_scrollTo == row.id) {
            ImGui::SetScrollHereY();
            _scrollTo = HierarchyModel::InvalidId;
        }

        // Dragging only reparents: a flat scene has nothing to drag.
        const bool parenting = _model.supportsParenting();
        if (parenting && ImGui::BeginDragDropSource()) {
            ImGui::SetDragDropPayload(DragPayloadType, &row.id, sizeof(row.id));
            ImGui::TextUnformatted(name.c_str());
            ImGui::EndDragDropSource();
        }
        if (parenting && ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(DragPayloadType)) {
                const ObjectId dragged = *static_cast<const ObjectId *>(payload->Data);
                const ObjectId target = row.id;
                _pending.emplace_back([this, dragged, target] { moveObject(dragged, target); });
            }
            ImGui::EndDragDropTarget();
        }
        drawContextMenu(row.id);
    }

    if (indent > 0.0f)
        ImGui::Unindent(indent);
    ImGui::PopID();
}

void HierarchyPanel::drawRenameField(const Row &row)
{
    // Aligned with the names of the other rows (after the arrow).
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetTreeNodeToLabelSpacing());
    ImGui::SetNextItemWidth(-FLT_MIN);
    if (_focusRename) {
        ImGui::SetKeyboardFocusHere();
        _focusRename = false;
    }
    const bool submitted = ImGui::InputText(
        "##Rename", &_renameBuffer, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);
    if (!submitted && !ImGui::IsItemDeactivated())
        return;

    // Enter or clicking elsewhere keeps the new name; Escape cancels.
    const bool cancelled = ImGui::IsKeyPressed(ImGuiKey_Escape);
    const ObjectId id = row.id;
    const std::string newName = _renameBuffer;
    _renaming = HierarchyModel::InvalidId;
    if (!cancelled && !newName.empty() && newName != _model.name(id))
        _pending.emplace_back([this, id, newName] { _model.rename(id, newName); });
}

void HierarchyPanel::drawContextMenu(ObjectId id)
{
    if (!ImGui::BeginPopupContextItem("##ObjectMenu"))
        return;
    select(id);
    if (_model.supportsParenting() && ImGui::MenuItem("Create Child"))
        createObject(id);
    if (ImGui::MenuItem("Rename", "F2"))
        startRename(id);
    ImGui::Separator();
    if (ImGui::MenuItem("Delete", "Del"))
        requestDelete(id);
    ImGui::EndPopup();
}

void HierarchyPanel::handleKeyboard(const std::vector<Row> &rows)
{
    if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) || _renaming != HierarchyModel::InvalidId
        || ImGui::GetIO().WantTextInput || !_selection.has())
        return;

    const ObjectId selected = _selection.get();
    if (ImGui::IsKeyPressed(ImGuiKey_Delete))
        requestDelete(selected);
    if (ImGui::IsKeyPressed(ImGuiKey_F2))
        startRename(selected);

    const auto it = std::ranges::find(rows, selected, &Row::id);
    if (it == rows.end())
        return;
    if (ImGui::IsKeyPressed(ImGuiKey_DownArrow) && it + 1 != rows.end()) {
        select((it + 1)->id);
        _scrollTo = (it + 1)->id;
    }
    if (ImGui::IsKeyPressed(ImGuiKey_UpArrow) && it != rows.begin()) {
        select((it - 1)->id);
        _scrollTo = (it - 1)->id;
    }
    if (ImGui::IsKeyPressed(ImGuiKey_RightArrow) && it->hasChildren)
        _expanded.insert(selected);
    if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) {
        // Collapse, or go up to the parent if already collapsed.
        if (it->hasChildren && _expanded.contains(selected)) {
            _expanded.erase(selected);
        } else if (_model.parent(selected) != HierarchyModel::InvalidId) {
            select(_model.parent(selected));
            _scrollTo = _model.parent(selected);
        }
    }
}
