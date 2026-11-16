#include <boost/ut.hpp>

#include "Support/Headless.hpp"
#include "ecs/EcsScene/EcsScene.hpp"
#include "ecs/TestComponents.hpp"
#include "panels/InspectorPanel/InspectorPanel.hpp"
#include "scene/DemoScene/DemoScene.hpp"
#include "ui/NotificationManager/NotificationManager.hpp"

#include <string>
#include <vector>

using namespace boost::ut;
using namespace EditorTest;
using ObjectId = HierarchyModel::ObjectId;

namespace {

struct InspectorFixture
{
    Gui gui;
    HierarchyModel &hierarchy;
    PropertyModel &properties;
    Selection selection;
    NotificationManager notifications;
    InspectorPanel panel;

    InspectorFixture(HierarchyModel &hierarchy, PropertyModel &properties)
        : hierarchy(hierarchy), properties(properties), panel(hierarchy, properties, selection, notifications)
    {
    }

    void frame(int count = 1)
    {
        gui.frames(count, [this] {
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(ImVec2(500, 880));
            panel.draw();
        });
    }

    ImGuiWindow *inspector() const { return window("Inspector"); }
    // A value widget: component, then the path of fields (groups are tree nodes).
    ImGuiID valueId(const std::string &component, const std::vector<std::string> &path, const char *item = "##Value") const
    {
        std::vector<IdPart> parts{component, "##Properties"};
        for (std::size_t i = 0; i < path.size(); i++) {
            parts.emplace_back(path[i]);
            if (i + 1 < path.size())
                parts.emplace_back(path[i]); // The group's tree node pushes its label
        }
        parts.emplace_back(std::string(item));
        return itemId(inspector(), parts);
    }
    ImGuiWindow *openMenu(ImGuiID popupId)
    {
        openPopup(popupId);
        frame(2);
        return popupWindow(popupId);
    }
};

} // namespace

void run_inspector_panel_tests()
{
    "nothing selected: a hint; a banner above the properties when set"_test = [] {
        DemoScene scene(true);
        InspectorFixture f(scene, scene);
        f.frame(2);
        expect(f.inspector() != nullptr);
        f.selection.set(scene.roots().front());
        f.panel.setBanner([] { return std::string("Play mode"); });
        f.frame(2);
        f.panel.setBanner([] { return std::string(); });
        f.frame();
    };

    "the name: edited in place, an empty name is refused"_test = [] {
        DemoScene scene;
        const ObjectId id = scene.create(HierarchyModel::InvalidId, "Box");
        InspectorFixture f(scene, scene);
        f.selection.set(id);
        f.frame(2);
        activate(itemId(f.inspector(), {"##Name"}));
        f.frame(2);
        press(ImGuiKey_End); // Activated, the text is selected
        f.frame(2);
        typeText("2");
        f.frame();
        press(ImGuiKey_Enter);
        f.frame(3);
        expect(scene.name(id) == "Box2") << scene.name(id);

        activate(itemId(f.inspector(), {"##Name"}));
        f.frame(2);
        press(ImGuiMod_Ctrl | ImGuiKey_A);
        f.frame(2);
        press(ImGuiKey_Backspace);
        f.frame(2);
        press(ImGuiKey_Enter);
        f.frame(3);
        expect(scene.name(id) == "Box2");
        scene.rename(id, "Renamed elsewhere");
        f.frame(2);
    };

    "values: checkbox, numbers typed in, text, enum, color, groups"_test = [] {
        DemoScene scene;
        const ObjectId id = scene.create(HierarchyModel::InvalidId, "Thing");
        scene.addComponent(id, "Sprite");
        scene.addComponent(id, "RigidBody");
        scene.addComponent(id, "PlayerController");
        scene.addComponent(id, "Tilemap");
        InspectorFixture f(scene, scene);
        f.selection.set(id);
        f.frame(3);

        activate(f.valueId("Sprite", {"flipX"}));
        f.frame(2);
        expect(scene.get(id, "Sprite", "flipX") == PropertyValue(true));

        activate(f.valueId("Sprite", {"layer"}), true);
        f.frame(2);
        press(ImGuiMod_Ctrl | ImGuiKey_A);
        f.frame(2);
        typeText("42");
        f.frame();
        press(ImGuiKey_Enter);
        f.frame(3);
        expect(scene.get(id, "Sprite", "layer") == PropertyValue(42));

        activate(f.valueId("RigidBody", {"mass"}), true);
        f.frame(2);
        press(ImGuiMod_Ctrl | ImGuiKey_A);
        f.frame(2);
        typeText("5");
        f.frame();
        press(ImGuiKey_Enter);
        f.frame(3);
        expect(scene.get(id, "RigidBody", "mass") == PropertyValue(5.0f));

        activate(f.valueId("Sprite", {"texture"}));
        f.frame(2);
        typeText("hero.png");
        f.frame(2);
        press(ImGuiKey_Enter);
        f.frame(2);
        expect(scene.get(id, "Sprite", "texture") == PropertyValue(std::string("hero.png")));

        // Enum: a dropdown of its options.
        activate(f.valueId("RigidBody", {"bodyType"}));
        f.frame(2);
        ImGuiWindow *combo = window("##Combo_00");
        expect(combo != nullptr);
        activate(itemId(combo, {"Kinematic"}));
        f.frame(3);
        expect(scene.get(id, "RigidBody", "bodyType") == PropertyValue(2));

        // A field inside a group.
        activate(f.valueId("PlayerController", {"movement", "speed"}), true);
        f.frame(2);
        press(ImGuiMod_Ctrl | ImGuiKey_A);
        f.frame(2);
        typeText("7");
        f.frame();
        press(ImGuiKey_Enter);
        f.frame(3);
        expect(scene.get(id, "PlayerController", "movement.speed") == PropertyValue(7.0f));

        // A group can be collapsed.
        activate(itemId(f.inspector(), {"PlayerController", "##Properties", "movement", "movement"}));
        f.frame(2);
        // A component header too.
        activate(itemId(f.inspector(), {"Sprite", "Sprite"}));
        f.frame(2);
    };

    "add a component from the searchable list, remove one from its menu"_test = [] {
        DemoScene scene;
        const ObjectId id = scene.create(HierarchyModel::InvalidId, "Thing");
        InspectorFixture f(scene, scene);
        f.selection.set(id);
        f.frame(2);
        activate(itemId(f.inspector(), {"Add Component"}));
        f.frame(2);
        const ImGuiID popupId = itemId(f.inspector(), {"##AddComponent"});
        ImGuiWindow *popup = popupWindow(popupId);
        expect(popup != nullptr);
        typeText("audio");
        f.frame(2);
        activate(itemId(popup, {"AudioSource"}));
        f.frame(3);
        expect(scene.get(id, "AudioSource", "volume") == PropertyValue(0.8f));

        activate(itemId(f.inspector(), {"Add Component"}));
        f.frame(2);
        typeText("zzz"); // Nothing matches
        f.frame(2);
        press(ImGuiKey_Escape);
        f.frame(2);

        ImGuiWindow *menu = f.openMenu(itemId(f.inspector(), {"AudioSource", "##ComponentMenu"}));
        expect(menu != nullptr);
        activate(itemId(menu, {"Remove Component"}));
        f.frame(3);
        expect(std::holds_alternative<std::monostate>(scene.get(id, "AudioSource", "volume")));

        // Transform cannot be removed: its item is disabled.
        menu = f.openMenu(itemId(f.inspector(), {"Transform", "##ComponentMenu"}));
        expect(menu != nullptr);
        press(ImGuiKey_Escape);
        f.frame(2);
    };

    "an ECS entity: every kind of field, read-only summaries, no components"_test = [] {
        ECS::Registry world;
        EcsScene scene(world);
        const ECS::Entity entity = world.spawnEntity("Hero");
        world.addComponent(entity, EdStats{});
        world.addComponent(entity, EdShape{});
        world.addComponent(entity, EdContainers{});
        InspectorFixture f(scene, scene);
        f.selection.set(scene.id(entity));
        f.frame(3);

        activate(f.valueId("EdStats", {"alive"}));
        f.frame(2);
        expect(!world.getComponent<EdStats>(entity).alive);
        activate(f.valueId("EdStats", {"title"}));
        f.frame(2);
        typeText("Sir");
        f.frame(2);
        expect(world.getComponent<EdStats>(entity).title == "Sir");
        press(ImGuiKey_Escape);
        f.frame(2);
        activate(f.valueId("EdStats", {"kind"}), true);
        f.frame(2);
        press(ImGuiMod_Ctrl | ImGuiKey_A);
        f.frame(2);
        typeText("2");
        f.frame();
        press(ImGuiKey_Enter);
        f.frame(3);
        expect(world.getComponent<EdStats>(entity).kind == EdKind::Boss);

        const ECS::Entity bare = world.spawnEntity("Bare");
        f.selection.set(scene.id(bare));
        f.frame(2);
    };
}
