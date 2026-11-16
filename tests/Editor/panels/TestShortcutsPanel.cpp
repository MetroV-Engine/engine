#include <boost/ut.hpp>

#include "Support/Headless.hpp"
#include "navigation/ActionRegistry/ActionRegistry.hpp"
#include "panels/ShortcutsPanel/ShortcutsPanel.hpp"
#include "ui/DialogManager/DialogManager.hpp"
#include "ui/NotificationManager/NotificationManager.hpp"

#include <string>

using namespace boost::ut;
using namespace EditorTest;

namespace {

struct ShortcutsFixture
{
    Gui gui;
    ActionRegistry actions;
    DialogManager dialogs;
    NotificationManager notifications;
    ShortcutsPanel panel{actions, dialogs, notifications};

    ShortcutsFixture()
    {
        actions.add({.id = "view.palette", .label = "Command Palette", .callback = [] {},
            .shortcut = ImGuiMod_Ctrl | ImGuiKey_P});
        actions.add({.id = "file.quit", .label = "Quit", .callback = [] {}, .shortcut = ImGuiMod_Ctrl | ImGuiKey_Q});
        actions.add({.id = "edit.rename", .label = "Rename", .callback = [] {}, .shortcutHint = "F2"});
        actions.add({.id = "edit.none", .label = "Nothing bound", .callback = [] {}});
        actions.add({.id = "view.hidden", .label = "No callback"});
    }

    void frame(int count = 1)
    {
        gui.frames(count, [this] {
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(ImVec2(900, 880));
            panel.draw();
            dialogs.draw();
        });
    }
    // A button of an action's row, in its category's table.
    ImGuiID rowButton(const std::string &id, const std::string &button) const
    {
        const ImGuiWindow *list = window("Keyboard Shortcuts/##Shortcuts_");
        return itemId(itemId(list, {ActionRegistry::categoryOf(id)}), {id, button});
    }
    // Clicks + on the row, then presses chord.
    void record(const std::string &id, ImGuiKeyChord chord)
    {
        activate(rowButton(id, "+"));
        frame(2);
        expect(panel.isRecording());
        press(chord);
        frame(3);
    }
};

} // namespace

void run_shortcuts_panel_tests()
{
    "record a shortcut: added; already this one's; reserved; Escape cancels"_test = [] {
        ShortcutsFixture f;
        f.frame(2);
        f.record("view.palette", ImGuiMod_Ctrl | ImGuiKey_K);
        expect(!f.panel.isRecording());
        expect(f.actions.findByShortcut(ImGuiMod_Ctrl | ImGuiKey_K) == f.actions.get("view.palette"));

        f.record("view.palette", ImGuiMod_Ctrl | ImGuiKey_K); // Already its shortcut
        f.record("view.palette", ImGuiKey_Space);              // Reserved
        expect(f.actions.findByShortcut(ImGuiKey_Space) == nullptr);
        expect(f.notifications.count() == 3_u);

        activate(f.rowButton("edit.none", "+"));
        f.frame(2);
        press(ImGuiKey_Escape);
        f.frame(3);
        expect(!f.panel.isRecording());
        expect(f.actions.shortcuts("edit.none").empty());
    };

    "a shortcut used by another command is moved only after confirmation"_test = [] {
        ShortcutsFixture f;
        f.frame(2);
        f.record("edit.none", ImGuiMod_Ctrl | ImGuiKey_Q);
        expect(f.dialogs.isOpen());
        press(ImGuiKey_Escape); // Keep it on Quit
        f.frame(3);
        expect(f.actions.findByShortcut(ImGuiMod_Ctrl | ImGuiKey_Q) == f.actions.get("file.quit"));

        f.record("edit.none", ImGuiMod_Ctrl | ImGuiKey_Q);
        press(ImGuiKey_Enter); // Reassign
        f.frame(3);
        expect(f.actions.findByShortcut(ImGuiMod_Ctrl | ImGuiKey_Q) == f.actions.get("edit.none"));
    };

    "click a shortcut to remove it; Reset and Reset All restore the defaults"_test = [] {
        ShortcutsFixture f;
        f.frame(2);
        activate(f.rowButton("view.palette", "Ctrl+P##0"));
        f.frame(3);
        expect(f.actions.shortcuts("view.palette").empty());
        activate(f.rowButton("view.palette", "Reset"));
        f.frame(2);
        expect(f.actions.hasDefaultShortcuts("view.palette"));

        f.actions.removeShortcut("file.quit", ImGuiMod_Ctrl | ImGuiKey_Q);
        f.actions.addShortcut("edit.rename", ImGuiKey_F8);
        f.frame(2);
        activate(itemId(window("Keyboard Shortcuts"), {"Reset All"}));
        f.frame(2);
        expect(f.dialogs.isOpen());
        press(ImGuiKey_Enter);
        f.frame(3);
        expect(f.actions.hasDefaultShortcuts("file.quit"));
        expect(f.actions.hasDefaultShortcuts("edit.rename"));
    };

    "the search filters commands, shortcuts and the reference tables"_test = [] {
        ShortcutsFixture f;
        f.frame(2);
        for (const char *search : {"ctrl+p", "edit", "hierarchy", "nothing at all matches"}) {
            activate(itemId(window("Keyboard Shortcuts"), {"##Filter"}));
            f.frame(2);
            press(ImGuiMod_Ctrl | ImGuiKey_A);
            f.frame(2);
            typeText(search);
            f.frame(3);
        }
        press(ImGuiKey_Escape);
        f.frame(2);
    };
}
