#include <boost/ut.hpp>

#include "Support/Headless.hpp"
#include "navigation/ActionRegistry/ActionRegistry.hpp"
#include "navigation/CommandPalette/CommandPalette.hpp"
#include "navigation/MenuBar/MenuBar.hpp"
#include "navigation/Toolbar/Toolbar.hpp"

#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace boost::ut;
using namespace EditorTest;

namespace {

void writeFile(const std::string &path, const std::string &content)
{
    std::ofstream(path, std::ios::trunc) << content;
}

std::string readFile(const std::string &path)
{
    std::ifstream file(path);
    return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

} // namespace

void run_action_registry_tests()
{
    "add: ids are unique and not empty; a shared default shortcut is a warning"_test = [] {
        ActionRegistry actions;
        int runs = 0;
        Action &quit = actions.add({.id = "file.quit", .label = "Quit", .callback = [&] { runs++; },
            .shortcut = ImGuiMod_Ctrl | ImGuiKey_Q});
        expect(quit.label == "Quit");
        expect(throws<std::invalid_argument>([&] { actions.add({.id = ""}); }));
        expect(throws<std::invalid_argument>([&] { actions.add({.id = "file.quit"}); }));
        LogWatch log;
        actions.add({.id = "file.exit", .label = "Exit", .shortcut = ImGuiMod_Ctrl | ImGuiKey_Q});
        expect(log.contains(LogLevel::Warning, "Ctrl+Q is used by both \"file.quit\" and \"file.exit\""));

        expect(actions.get("file.quit") == &quit);
        expect(actions.get("nope") == nullptr);
        expect(actions.has("file.exit"));
        expect(actions.getActions().size() == 2_u);
    };

    "execute: only an existing, enabled action with a callback runs"_test = [] {
        ActionRegistry actions;
        int runs = 0;
        bool enabled = false;
        actions.add({.id = "a", .callback = [&] { runs++; }, .isEnabled = [&] { return enabled; },
            .isChecked = [] { return true; }});
        actions.add({.id = "nothing"});
        expect(!actions.execute("a"));
        enabled = true;
        expect(actions.execute("a"));
        expect(runs == 1_i);
        expect(!actions.execute("nothing"));
        expect(!actions.execute("missing"));
        const Action &a = *actions.get("a");
        expect(a.checkable() and a.checked() and a.enabled());
        const Action &nothing = *actions.get("nothing");
        expect(!nothing.checkable() and !nothing.checked() and nothing.enabled());
    };

    "shortcutText and parseShortcut are inverses; symbols and modifiers"_test = [] {
        const ImGuiKeyChord chords[] = {ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_S, ImGuiMod_Alt | ImGuiKey_F4,
            ImGuiMod_Super | ImGuiKey_GraveAccent, ImGuiMod_Ctrl | ImGuiKey_Comma, ImGuiKey_Delete, ImGuiKey_F1};
        for (const ImGuiKeyChord chord : chords) {
            const std::string text = ActionRegistry::shortcutText(chord);
            expect(ActionRegistry::parseShortcut(text) == std::optional<ImGuiKeyChord>(chord)) << text;
        }
        expect(ActionRegistry::shortcutText(ImGuiKey_None).empty());
        expect(ActionRegistry::shortcutText(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiMod_Alt | ImGuiMod_Super | ImGuiKey_A)
            == "Ctrl+Shift+Alt+Super+A");
        expect(ActionRegistry::parseShortcut(" control + shift + p ") == std::optional<ImGuiKeyChord>(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_P));
        expect(ActionRegistry::parseShortcut("cmd+k") == std::optional<ImGuiKeyChord>(ImGuiMod_Super | ImGuiKey_K));
        expect(ActionRegistry::parseShortcut("win+k") == std::optional<ImGuiKeyChord>(ImGuiMod_Super | ImGuiKey_K));
        expect(ActionRegistry::parseShortcut("Ctrl++") == std::nullopt); // "+" is not a key name
        expect(ActionRegistry::parseShortcut("Ctrl+=") == std::optional<ImGuiKeyChord>(ImGuiMod_Ctrl | ImGuiKey_Equal));
        expect(ActionRegistry::parseShortcut("Hyper+A") == std::nullopt);
        expect(ActionRegistry::parseShortcut("") == std::nullopt);
        expect(ActionRegistry::parseShortcut("Ctrl+") == std::nullopt);
        expect(ActionRegistry::parseShortcut("LeftCtrl") == std::nullopt); // A modifier alone
    };

    "reservedReason: navigation and text editing keys cannot be shortcuts"_test = [] {
        expect(ActionRegistry::reservedReason(ImGuiKey_Escape).has_value());
        expect(ActionRegistry::reservedReason(ImGuiKey_Tab).has_value());
        expect(ActionRegistry::reservedReason(ImGuiMod_Ctrl | ImGuiKey_Tab).has_value());
        expect(ActionRegistry::reservedReason(ImGuiMod_Shift | ImGuiKey_Enter).has_value());
        expect(ActionRegistry::reservedReason(ImGuiKey_MouseLeft).has_value());
        expect(*ActionRegistry::reservedReason(ImGuiKey_Space) == "Space is used to navigate and edit text");
        expect(!ActionRegistry::reservedReason(ImGuiMod_Ctrl | ImGuiKey_Space).has_value());
        expect(!ActionRegistry::reservedReason(ImGuiMod_Alt | ImGuiKey_Tab).has_value());
        expect(!ActionRegistry::reservedReason(ImGuiKey_F5).has_value());
    };

    "categoryOf: the display group of an action id"_test = [] {
        expect(ActionRegistry::categoryOf("view.panel.Console") == "Panel");
        expect(ActionRegistry::categoryOf("view.theme.Dark") == "Theme");
        expect(ActionRegistry::categoryOf("window.focus.Viewport") == "Window");
        expect(ActionRegistry::categoryOf("file.quit") == "File");
        expect(ActionRegistry::categoryOf("edit.create") == "Edit");
        expect(ActionRegistry::categoryOf("view.palette") == "View");
        expect(ActionRegistry::categoryOf("help.shortcuts") == "Help");
        expect(ActionRegistry::categoryOf("game.play") == "Command");
    };

    "shortcuts: added, removed, reset; each change is notified"_test = [] {
        ActionRegistry actions;
        int changes = 0;
        actions.setOnShortcutsChanged([&] { changes++; });
        actions.add({.id = "view.palette", .label = "Palette", .shortcut = ImGuiMod_Ctrl | ImGuiKey_P,
            .shortcutHint = "unused"});
        actions.add({.id = "edit.rename", .label = "Rename", .shortcutHint = "F2"});
        const ImGuiKeyChord ctrlK = ImGuiMod_Ctrl | ImGuiKey_K;

        expect(actions.hasDefaultShortcuts("view.palette"));
        expect(actions.addShortcut("view.palette", ctrlK));
        expect(!actions.addShortcut("view.palette", ctrlK));         // Already bound
        expect(!actions.addShortcut("view.palette", ImGuiKey_LeftCtrl)); // Not a shortcut key
        expect(!actions.addShortcut("missing", ctrlK));
        expect(actions.shortcuts("view.palette").size() == 2_u);
        expect(!actions.hasDefaultShortcuts("view.palette"));
        expect(actions.findByShortcut(ctrlK) == actions.get("view.palette"));
        expect(actions.findByShortcut(ImGuiKey_F9) == nullptr);
        expect(actions.shortcutLabel(*actions.get("view.palette")) == "Ctrl+P");
        expect(actions.shortcutLabel(*actions.get("edit.rename")) == "F2");

        expect(actions.removeShortcut("view.palette", ImGuiMod_Ctrl | ImGuiKey_P));
        expect(!actions.removeShortcut("view.palette", ImGuiMod_Ctrl | ImGuiKey_P));
        expect(!actions.removeShortcut("missing", ctrlK));
        actions.resetShortcuts("view.palette");
        actions.resetShortcuts("missing");
        expect(actions.hasDefaultShortcuts("view.palette"));
        expect(actions.hasDefaultShortcuts("missing"));
        actions.addShortcut("edit.rename", ImGuiKey_F2);
        actions.resetAllShortcuts();
        expect(actions.shortcuts("edit.rename").empty());
        expect(actions.shortcuts("missing").empty());
        expect(changes == 5_i);
    };

    "save and load: only the shortcuts that differ from the defaults"_test = [] {
        ActionRegistry actions;
        actions.add({.id = "view.palette", .label = "Palette", .shortcut = ImGuiMod_Ctrl | ImGuiKey_P});
        actions.add({.id = "view.console", .label = "Console", .shortcut = ImGuiKey_GraveAccent});
        actions.add({.id = "file.quit", .label = "Quit", .shortcut = ImGuiMod_Ctrl | ImGuiKey_Q});
        actions.addShortcut("view.palette", ImGuiMod_Ctrl | ImGuiKey_Comma);
        actions.removeShortcut("view.console", ImGuiKey_GraveAccent);
        expect(actions.saveShortcuts("shortcuts-test.ini"));
        const std::string saved = readFile("shortcuts-test.ini");
        expect(saved.find("view.palette=Ctrl+P,Ctrl+,\n") != std::string::npos) << saved;
        expect(saved.find("view.console=\n") != std::string::npos);
        expect(saved.find("file.quit") == std::string::npos);

        ActionRegistry loaded;
        loaded.add({.id = "view.palette", .label = "Palette", .shortcut = ImGuiMod_Ctrl | ImGuiKey_P});
        loaded.add({.id = "view.console", .label = "Console", .shortcut = ImGuiKey_GraveAccent});
        LogWatch log;
        loaded.loadShortcuts("shortcuts-test.ini");
        expect(loaded.shortcuts("view.palette") == actions.shortcuts("view.palette"));
        expect(loaded.shortcuts("view.console").empty());
        expect(log.contains("Loaded 2 custom shortcut(s)"));

        expect(!actions.saveShortcuts("no/such/directory/shortcuts.ini"));
        loaded.loadShortcuts("no-such-file.ini"); // Not an error
    };

    "load: comments, malformed lines, unknown actions and typos are skipped"_test = [] {
        writeFile("shortcuts-bad.ini", "# comment\n\n"
                                       "no equal sign\n"
                                       "unknown.action=F5\n"
                                       "view.palette=Ctrl+K, Typo+Nope\n"
                                       "  view.console = F8 , ,F9  \n");
        ActionRegistry actions;
        actions.add({.id = "view.palette", .label = "Palette", .shortcut = ImGuiMod_Ctrl | ImGuiKey_P});
        actions.add({.id = "view.console", .label = "Console"});
        LogWatch log;
        actions.loadShortcuts("shortcuts-bad.ini");
        expect(log.contains(LogLevel::Warning, "shortcuts-bad.ini:3: expected <action id>=<shortcuts>"));
        expect(log.contains(LogLevel::Warning, "unknown action \"unknown.action\""));
        expect(log.contains(LogLevel::Warning, "invalid shortcut \"Typo+Nope\", line ignored"));
        expect(actions.hasDefaultShortcuts("view.palette")); // The line with a typo changes nothing
        expect(actions.shortcuts("view.console") == std::vector<ImGuiKeyChord>{ImGuiKey_F8, ImGuiKey_F9});
    };

    "processShortcuts: a pressed chord runs its action; plain keys wait while typing"_test = [] {
        Gui gui;
        ActionRegistry actions;
        int palette = 0;
        int plain = 0;
        int function = 0;
        actions.add({.id = "view.palette", .callback = [&] { palette++; }, .shortcut = ImGuiMod_Ctrl | ImGuiKey_P});
        actions.add({.id = "plain", .callback = [&] { plain++; }, .shortcut = ImGuiKey_K});
        actions.add({.id = "function", .callback = [&] { function++; }, .shortcut = ImGuiKey_F7});
        actions.add({.id = "disabled", .callback = [&] { plain += 100; }, .shortcut = ImGuiKey_J,
            .isEnabled = [] { return false; }});
        std::string text;
        bool focusField = false;
        const auto ui = [&] {
            actions.processShortcuts();
            ImGui::Begin("Typing");
            if (focusField) {
                ImGui::SetKeyboardFocusHere();
                focusField = false;
            }
            char buffer[32] = {};
            ImGui::InputText("##Field", buffer, sizeof(buffer));
            ImGui::End();
        };
        gui.frame(ui);
        press(ImGuiMod_Ctrl | ImGuiKey_P);
        gui.frames(2, ui);
        press(ImGuiKey_K);
        gui.frames(2, ui);
        press(ImGuiKey_J);
        gui.frames(2, ui);
        expect(palette == 1_i and plain == 1_i);

        focusField = true;
        gui.frames(2, ui);
        press(ImGuiKey_K);
        gui.frames(2, ui);
        press(ImGuiKey_F7);
        gui.frames(2, ui);
        press(ImGuiMod_Ctrl | ImGuiKey_P);
        gui.frames(2, ui);
        expect(plain == 1_i);
        expect(function == 1_i);
        expect(palette == 2_i);
    };
}

void run_command_palette_tests()
{
    "fuzzyScore: letters in order, better at word starts and in a row"_test = [] {
        expect(CommandPalette::fuzzyScore("", "anything") == 0_i);
        expect(CommandPalette::fuzzyScore("xyz", "View: Fullscreen") == -1_i);
        expect(CommandPalette::fuzzyScore("fsc", "View: Fullscreen") > 0_i);
        expect(CommandPalette::fuzzyScore("full", "View: Fullscreen") > CommandPalette::fuzzyScore("fsc", "View: Fullscreen"));
        expect(CommandPalette::fuzzyScore("v f", "View: Fullscreen") > 0_i);
        expect(CommandPalette::fuzzyScore("Vi", "View") > CommandPalette::fuzzyScore("Vw", "View"));
    };

    "the palette: opens, filters, moves with Up/Down, runs with Enter, closes"_test = [] {
        Gui gui;
        ActionRegistry actions;
        std::vector<std::string> ran;
        actions.add({.id = "view.fullscreen", .label = "Fullscreen", .callback = [&] { ran.push_back("fullscreen"); },
            .shortcut = ImGuiKey_F11, .isChecked = [] { return true; }});
        actions.add({.id = "view.performance", .label = "Performance", .callback = [&] { ran.push_back("performance"); }});
        actions.add({.id = "view.disabled", .label = "Disabled", .callback = [&] { ran.push_back("disabled"); },
            .isEnabled = [] { return false; }});
        actions.add({.id = "view.nothing", .label = "No callback"});
        CommandPalette palette(actions);
        const auto ui = [&] { palette.draw(); };

        gui.frame(ui);
        expect(!palette.isOpen());
        palette.toggle();
        gui.frames(2, ui);
        expect(palette.isOpen());
        typeText("fsc");
        gui.frame(ui);
        press(ImGuiKey_Enter);
        gui.frames(3, ui);
        expect(ran == std::vector<std::string>{"fullscreen"});
        expect(!palette.isOpen());

        // Recent commands come first; Down selects the next one.
        palette.toggle();
        gui.frames(2, ui);
        press(ImGuiKey_DownArrow);
        gui.frames(2, ui);
        press(ImGuiKey_UpArrow);
        gui.frames(2, ui);
        press(ImGuiKey_DownArrow);
        gui.frames(2, ui);
        press(ImGuiKey_Enter);
        gui.frames(3, ui);
        expect(ran.back() == "performance");

        // No match, then Escape closes it.
        palette.toggle();
        gui.frames(2, ui);
        typeText("zzz");
        gui.frames(2, ui);
        press(ImGuiKey_Enter);
        gui.frames(2, ui);
        expect(palette.isOpen());
        press(ImGuiKey_Escape);
        gui.frames(3, ui);
        expect(!palette.isOpen());

        // A disabled command does not run from Enter; toggling again closes.
        palette.toggle();
        gui.frames(2, ui);
        typeText("Disabled");
        gui.frame(ui);
        press(ImGuiKey_Enter);
        gui.frames(3, ui);
        expect(std::ranges::count(ran, "disabled") == 0);
        palette.toggle();
        gui.frames(2, ui);
        expect(!palette.isOpen());
    };

    "clicking a command runs it; the recent list keeps the last five"_test = [] {
        Gui gui;
        ActionRegistry actions;
        std::vector<std::string> ran;
        for (int i = 0; i < 7; i++) {
            const std::string id = "command." + std::to_string(i);
            actions.add({.id = id, .label = "Command " + std::to_string(i), .callback = [&ran, id] { ran.push_back(id); }});
        }
        CommandPalette palette(actions);
        const auto ui = [&] { palette.draw(); };
        gui.frame(ui);
        for (int i = 0; i < 7; i++) {
            palette.toggle();
            gui.frames(2, ui);
            const ImGuiID popup = itemId(window("Debug##Default"), {"##CommandPalette"});
            ImGuiWindow *list = window(std::string(popupWindow(popup)->Name) + "/##Commands_");
            expect(list != nullptr);
            const std::string id = "command." + std::to_string(i);
            const std::string label = CommandPalette::displayName(*actions.get(id));
            activate(itemId(list, {id, label}));
            gui.frames(3, ui);
            expect(ran.back() == id);
        }
    };
}

void run_menu_bar_tests()
{
    "menus by path, in the order first used; unknown actions and empty paths are refused"_test = [] {
        ActionRegistry actions;
        actions.add({.id = "file.quit", .label = "Quit"});
        MenuBar menu(actions);
        menu.addAction("File", "file.quit");
        expect(throws<std::invalid_argument>([&] { menu.addAction("File", "missing"); }));
        expect(throws<std::invalid_argument>([&] { menu.addSeparator("//"); }));
        expect(menu.isVisible());
        menu.setVisible(false);
        expect(!menu.isVisible());
    };

    "drawing: menus, submenus, separators, custom entries; a clicked item runs its action"_test = [] {
        Gui gui;
        ActionRegistry actions;
        int quits = 0;
        int customs = 0;
        bool checked = true;
        actions.add({.id = "file.quit", .label = "Quit", .callback = [&] { quits++; }, .shortcut = ImGuiMod_Ctrl | ImGuiKey_Q});
        actions.add({.id = "view.toggle", .label = "Toggle", .callback = [&] { checked = !checked; },
            .isChecked = [&] { return checked; }});
        actions.add({.id = "view.off", .label = "Off", .callback = [] {}, .isEnabled = [] { return false; }});
        MenuBar menu(actions);
        menu.addAction("File", "file.quit");
        menu.addSeparator("File");
        menu.addCustom("File", [&] { customs++; });
        menu.addCustom("File", {});
        menu.addAction("View/Options", "view.toggle");
        menu.addAction("View", "view.off");
        const auto ui = [&] { menu.draw(); };

        gui.frame(ui);
        const ImGuiID bar = itemId(window("##MainMenuBar"), {"##MenuBar"});
        activate(itemId(bar, {"File"}));
        gui.frames(2, ui);
        expect(customs > 0_i);
        activate(itemId(menuWindow(0), {"Quit##file.quit"}));
        gui.frames(2, ui);
        expect(quits == 1_i);

        activate(itemId(bar, {"View"}));
        gui.frames(2, ui);
        activate(itemId(menuWindow(0), {"Options"}));
        gui.frames(2, ui);
        expect(menuWindow(1) != nullptr);
        activate(itemId(menuWindow(1), {"Toggle##view.toggle"}));
        gui.frames(2, ui);
        expect(!checked);

        menu.setVisible(false);
        gui.frame(ui);
    };
}

void run_toolbar_tests()
{
    "buttons run their action; toggles are highlighted; a hovered button shows its tooltip"_test = [] {
        Gui gui;
        ActionRegistry actions;
        int clicks = 0;
        bool on = true;
        actions.add({.id = "view.toggle", .label = "Toggle", .callback = [&] { on = !on; }, .shortcut = ImGuiKey_F3,
            .tooltip = "Toggle it", .isChecked = [&] { return on; }});
        actions.add({.id = "view.plain", .label = "Plain", .callback = [&] { clicks++; }});
        Toolbar toolbar(actions);
        toolbar.addAction("view.toggle");
        toolbar.addSeparator();
        toolbar.addAction("view.plain");
        expect(throws<std::invalid_argument>([&] { toolbar.addAction("missing"); }));
        const auto ui = [&] { toolbar.draw(); };

        gui.frame(ui);
        const ImGuiWindow *bar = window("##Toolbar");
        expect(bar != nullptr);
        activate(itemId(bar, {"Plain##view.plain"}));
        gui.frames(2, ui);
        expect(clicks == 1_i);
        activate(itemId(bar, {"Toggle##view.toggle"}));
        gui.frames(2, ui);
        expect(!on);

        // The first button is at the left of the bar.
        mouseMove(ImVec2(bar->Pos.x + bar->WindowPadding.x + 4.0f, bar->Pos.y + bar->Size.y * 0.5f));
        for (int i = 0; i < 20; i++) {
            SDL_Delay(15);
            gui.frame(ui);
        }
        expect(window("##Tooltip_00") != nullptr);

        toolbar.setVisible(false);
        expect(!toolbar.isVisible());
        gui.frame(ui);
    };
}
