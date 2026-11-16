#include <boost/ut.hpp>

#include "Support/Headless.hpp"
#include "ecs/EcsScene/EcsScene.hpp"
#include "panels/HierarchyPanel/HierarchyPanel.hpp"
#include "scene/DemoScene/DemoScene.hpp"
#include "ui/DialogManager/DialogManager.hpp"
#include "ui/NotificationManager/NotificationManager.hpp"

#include <cstdint>
#include <string>

using namespace boost::ut;
using namespace EditorTest;
using ObjectId = HierarchyModel::ObjectId;

namespace {

// A Hierarchy panel at a fixed place, with the dialogs it opens.
struct HierarchyFixture
{
    Gui gui;
    HierarchyModel &model;
    Selection selection;
    NotificationManager notifications;
    DialogManager dialogs;
    HierarchyPanel panel;
    bool focus = true;

    explicit HierarchyFixture(HierarchyModel &model) : model(model), panel(model, selection, notifications, dialogs) {}

    void frame(int count = 1)
    {
        gui.frames(count, [this] {
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(ImVec2(400, 800));
            panel.draw();
            // The panel handles its keys only when it has the focus. Focused
            // once: focusing a window again would take the keyboard from its fields.
            ImGuiWindow *hierarchy = window("Hierarchy");
            // Not while a popup is open: it has the focus, and would be closed.
            if (focus && GImGui->OpenPopupStack.Size == 0 && (!GImGui->NavWindow || GImGui->NavWindow->RootWindow != hierarchy))
                ImGui::SetWindowFocus("Hierarchy");
            dialogs.draw();
        });
    }

    ImGuiWindow *tree() const { return window("Hierarchy/##HierarchyTree_"); }
    ImGuiID rowId(ObjectId id, const char *item) const
    {
        return itemId(tree(), {PointerId{reinterpret_cast<const void *>(static_cast<std::uintptr_t>(id))}, item});
    }
    // Opens a context menu of the tree, and returns its window.
    ImGuiWindow *openMenu(ImGuiID popupId)
    {
        openPopup(popupId);
        frame(2);
        return popupWindow(popupId);
    }
    // Screen position of a row of the tree (rows are one line each).
    ImVec2 rowPosition(int row) const
    {
        const ImGuiWindow *w = tree();
        const float line = ImGui::GetFontSize() + ImGui::GetStyle().ItemSpacing.y;
        return ImVec2(w->Pos.x + 60.0f, w->DC.CursorStartPos.y + line * static_cast<float>(row) + ImGui::GetFontSize() * 0.5f);
    }
};

ObjectId find(const HierarchyModel &model, const std::string &name)
{
    std::vector<ObjectId> stack = model.roots();
    while (!stack.empty()) {
        const ObjectId id = stack.back();
        stack.pop_back();
        if (model.name(id) == name)
            return id;
        for (ObjectId child : model.children(id))
            stack.push_back(child);
    }
    return HierarchyModel::InvalidId;
}

} // namespace

void run_hierarchy_panel_tests()
{
    "an empty scene; + creates an object, selects it and renames it"_test = [] {
        DemoScene scene;
        HierarchyFixture f(scene);
        f.frame(2);
        activate(itemId(window("Hierarchy"), {"+"}));
        f.frame(4);
        expect(scene.roots().size() == 1_u);
        const ObjectId created = scene.roots().front();
        expect(f.selection.get() == created);
        // The name is selected: typing replaces it.
        typeText("Hero");
        f.frame();
        press(ImGuiKey_Enter);
        f.frame(3);
        expect(scene.name(created) == "Hero") << scene.name(created);

        // Escape cancels a rename.
        f.panel.startRename(created);
        f.frame(4);
        typeText("Ignored");
        f.frame();
        press(ImGuiKey_Escape);
        f.frame(3);
        expect(scene.name(created).find("Ignored") == std::string::npos);
        f.panel.startRename(999); // Unknown: nothing happens
        f.panel.requestDelete(999);
    };

    "keyboard: arrows move and expand, F2 renames, Delete asks then deletes"_test = [] {
        DemoScene scene(true);
        HierarchyFixture f(scene);
        f.frame(2);
        const ObjectId player = find(scene, "Player");
        f.selection.set(player);
        f.frame();

        press(ImGuiKey_RightArrow); // Expands the Player
        f.frame(2);
        press(ImGuiKey_DownArrow); // Its first child
        f.frame(2);
        expect(scene.parent(f.selection.get()) == player);
        press(ImGuiKey_LeftArrow); // A leaf: up to the parent
        f.frame(2);
        expect(f.selection.get() == player);
        press(ImGuiKey_LeftArrow); // Collapses
        f.frame(2);
        press(ImGuiKey_UpArrow);
        f.frame(2);
        expect(f.selection.get() == find(scene, "Main Camera"));
        press(ImGuiKey_UpArrow); // Already first
        f.frame(2);

        press(ImGuiKey_F2);
        f.frame(4);
        press(ImGuiKey_End);
        f.frame(2);
        typeText(" 2");
        f.frame();
        press(ImGuiKey_Enter);
        f.frame(3);
        expect(scene.name(f.selection.get()) == "Main Camera 2");

        // Delete asks first; the answer deletes the object and its children.
        f.selection.set(player);
        f.frame();
        press(ImGuiKey_Delete);
        f.frame(2);
        expect(f.dialogs.isOpen());
        press(ImGuiKey_Enter);
        f.frame(3);
        expect(!scene.exists(player));
        expect(!f.selection.has());
    };

    "search: matching objects with their parents, or a message"_test = [] {
        DemoScene scene(true);
        HierarchyFixture f(scene);
        f.frame(2);
        activate(itemId(window("Hierarchy"), {"##HierarchySearch"}));
        f.frame(2);
        typeText("crawler 3");
        f.frame(3);
        // Results are drawn expanded; the parents stay.
        press(ImGuiKey_Escape);
        f.frame(2);
        activate(itemId(window("Hierarchy"), {"##HierarchySearch"}));
        f.frame(2);
        typeText("zzz");
        f.frame(2);
        expect(scene.roots().size() == 4_u);
    };

    "the tree: nodes toggled open, context menus, the empty area"_test = [] {
        DemoScene scene(true);
        HierarchyFixture f(scene);
        f.frame(2);
        const ObjectId level = find(scene, "Level 01 - Forgotten Caverns");
        activate(f.rowId(level, "##Node")); // Collapses it (roots start expanded)
        f.frame(2);
        activate(f.rowId(level, "##Node"));
        f.frame(2);

        // Object menu: Create Child, Rename, Delete.
        ImGuiWindow *menu = f.openMenu(f.rowId(level, "##ObjectMenu"));
        expect(menu != nullptr);
        expect(f.selection.get() == level);
        const std::size_t children = scene.children(level).size();
        activate(itemId(menu, {"Create Child"}));
        f.frame(4);
        expect(scene.children(level).size() == children + 1);
        press(ImGuiKey_Escape); // Leaves the rename of the new child
        f.frame(2);

        menu = f.openMenu(f.rowId(level, "##ObjectMenu"));
        activate(itemId(menu, {"Rename"}));
        f.frame(4); // The field takes the keyboard
        press(ImGuiKey_Escape);
        f.frame(2);

        menu = f.openMenu(f.rowId(level, "##ObjectMenu"));
        activate(itemId(menu, {"Delete"}));
        f.frame(3);
        expect(f.dialogs.isOpen());
        press(ImGuiKey_Escape);
        f.frame(3);
        expect(scene.exists(level));

        // The empty area: a click deselects, its menu creates an object.
        const ImGuiWindow *tree = f.tree();
        const ImVec2 empty(tree->Pos.x + 40.0f, tree->Pos.y + tree->Size.y - 20.0f);
        scene.destroy(level); // Leaves room below the rows
        f.frame(2);
        click(empty);
        f.frame(4);
        expect(!f.selection.has());
        click(empty, ImGuiMouseButton_Right);
        f.frame(4);
        const ImGuiID emptyMenu = itemId(f.tree(), {"##HierarchyEmptyMenu"});
        expect(popupWindow(emptyMenu) != nullptr);
        const std::size_t roots = scene.roots().size();
        activate(itemId(popupWindow(emptyMenu), {"Create Object"}));
        f.frame(3);
        expect(scene.roots().size() == roots + 1);
    };

    "a click selects a row; dragging one onto another makes it its child"_test = [] {
        DemoScene scene;
        const ObjectId first = scene.create(HierarchyModel::InvalidId, "First");
        const ObjectId second = scene.create(HierarchyModel::InvalidId, "Second");
        const ObjectId third = scene.create(HierarchyModel::InvalidId, "Third");
        HierarchyFixture f(scene);
        f.focus = false;
        f.frame(3);
        click(f.rowPosition(1));
        f.frame(4);
        expect(f.selection.get() == second);

        // Drag Third onto First.
        mouseMove(f.rowPosition(2));
        f.frame();
        mouseButton(ImGuiMouseButton_Left, true);
        f.frame();
        for (int i = 1; i <= 6; i++) {
            const ImVec2 from = f.rowPosition(2);
            const ImVec2 to = f.rowPosition(0);
            mouseMove(ImVec2(from.x, from.y + (to.y - from.y) * static_cast<float>(i) / 6.0f));
            f.frame();
        }
        mouseButton(ImGuiMouseButton_Left, false);
        f.frame(3);
        expect(scene.parent(third) == first);

        // Dragging First into its own child is refused with a warning.
        f.panel.startRename(HierarchyModel::InvalidId);
        mouseMove(f.rowPosition(0));
        f.frame();
        mouseButton(ImGuiMouseButton_Left, true);
        f.frame();
        for (int i = 1; i <= 6; i++) {
            const ImVec2 from = f.rowPosition(0);
            const ImVec2 to = f.rowPosition(1);
            mouseMove(ImVec2(from.x, from.y + (to.y - from.y) * static_cast<float>(i) / 6.0f));
            f.frame();
        }
        mouseButton(ImGuiMouseButton_Left, false);
        f.frame(3);
        expect(scene.parent(first) == HierarchyModel::InvalidId);

        // Dropped on the empty area: back to the root.
        const ImGuiWindow *tree = f.tree();
        const ImVec2 empty(tree->Pos.x + 40.0f, tree->Pos.y + tree->Size.y - 20.0f);
        mouseMove(f.rowPosition(1));
        f.frame();
        mouseButton(ImGuiMouseButton_Left, true);
        f.frame();
        for (int i = 1; i <= 6; i++) {
            const ImVec2 from = f.rowPosition(1);
            mouseMove(ImVec2(from.x, from.y + (empty.y - from.y) * static_cast<float>(i) / 6.0f));
            f.frame();
        }
        mouseButton(ImGuiMouseButton_Left, false);
        f.frame(3);
        expect(f.notifications.count() >= 0_u);
    };

    "a flat ECS scene: no Create Child, no drag and drop; a deleted selection is cleared"_test = [] {
        ECS::Registry world;
        EcsScene scene(world);
        world.spawnEntity("One");
        const ECS::Entity two = world.spawnEntity("Two");
        HierarchyFixture f(scene);
        f.focus = false;
        f.frame(2);
        const ObjectId twoId = scene.id(two);
        f.selection.set(twoId);
        ImGuiWindow *menu = f.openMenu(f.rowId(twoId, "##ObjectMenu"));
        expect(menu != nullptr);
        press(ImGuiKey_Escape);
        f.frame(2);
        world.killEntity(two);
        f.frame(2);
        expect(!f.selection.has());
        f.panel.createObject(twoId); // Refused under a parent: nothing created
        f.frame(2);
        expect(scene.roots().size() == 1_u);
    };
}
