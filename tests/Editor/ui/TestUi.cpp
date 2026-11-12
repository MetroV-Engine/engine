#include <boost/ut.hpp>

#include "Support/Headless.hpp"
#include "ui/Button/Button.hpp"
#include "ui/Checkbox/Checkbox.hpp"
#include "ui/ComboBox/ComboBox.hpp"
#include "ui/DialogManager/DialogManager.hpp"
#include "ui/NotificationManager/NotificationManager.hpp"
#include "ui/NumberInput.hpp"
#include "ui/PerformanceOverlay/PerformanceOverlay.hpp"
#include "ui/RadioGroup/RadioGroup.hpp"
#include "ui/TextInput/TextInput.hpp"
#include "ui/Theme/Theme.hpp"
#include "ui/ThemeManager/ThemeManager.hpp"

#include <stdexcept>
#include <string>
#include <vector>

using namespace boost::ut;
using namespace EditorTest;

namespace {

// Draws widgets in a window of their own.
void inWindow(const std::function<void()> &widgets)
{
    ImGui::SetNextWindowPos(ImVec2(100, 100));
    ImGui::SetNextWindowSize(ImVec2(600, 500));
    ImGui::Begin("Widgets");
    widgets();
    ImGui::End();
}

ImGuiID widget(const std::vector<IdPart> &path)
{
    return itemId(window("Widgets"), path);
}

} // namespace

void run_widget_tests()
{
    "Button: draws in each variant, runs its callback when clicked, not when disabled"_test = [] {
        Gui gui;
        int clicks = 0;
        Button primary("OK", [&] { clicks++; });
        Button secondary("Later", {}, Button::Variant::Secondary);
        Button danger("Delete", {}, Button::Variant::Danger, ImVec2(120, 0));
        bool pressed = false;
        const auto ui = [&] {
            inWindow([&] {
                pressed = primary.render() || pressed;
                secondary.render();
                danger.render();
            });
        };
        gui.frame(ui);
        activate(widget({"OK"}));
        gui.frame(ui);
        expect(clicks == 1_i);
        expect(pressed);

        primary.setEnabled(false);
        expect(!primary.isEnabled());
        activate(widget({"OK"}));
        gui.frames(2, ui);
        expect(clicks == 1_i);

        primary.setLabel("Go");
        primary.setVariant(Button::Variant::Danger);
        primary.setSize(ImVec2(80, 0));
        primary.setOnClick({});
        primary.setEnabled(true);
        expect(primary.getLabel() == "Go");
        activate(widget({"Go"}));
        gui.frame(ui);
    };

    "Checkbox: toggles and reports the change"_test = [] {
        Gui gui;
        std::vector<bool> changes;
        Checkbox box("Enabled", false, [&](bool checked) { changes.push_back(checked); });
        const auto ui = [&] { inWindow([&] { box.render(); }); };
        gui.frame(ui);
        activate(widget({"Enabled"}));
        gui.frame(ui);
        expect(box.isChecked());
        expect(changes == std::vector<bool>{true});

        box.setChecked(false);
        box.setLabel("On");
        box.setOnChange({});
        box.setEnabled(false);
        expect(box.getLabel() == "On" and !box.isEnabled());
        gui.frame(ui);
    };

    "ComboBox: picks an option from its popup; out-of-range selections are ignored"_test = [] {
        Gui gui;
        std::string picked;
        ComboBox combo("Size", {"Small", "Medium", "Large"}, 1, [&](int, const std::string &option) { picked = option; });
        combo.setWidth(200.0f);
        const auto ui = [&] { inWindow([&] { combo.render(); }); };
        gui.frame(ui);
        activate(widget({"Size"}));
        gui.frame(ui);
        ImGuiWindow *popup = window("##Combo_00");
        expect(popup != nullptr);
        activate(itemId(popup, {2, "Large"}));
        gui.frames(2, ui);
        expect(picked == "Large");
        expect(combo.getSelected() == 2_i);
        expect(combo.getSelectedOption() == "Large");

        combo.setSelected(7);
        expect(combo.getSelected() == 2_i);
        combo.setOptions({"A"});
        expect(combo.getSelected() == 0_i);
        combo.setOptions({});
        expect(combo.getSelected() == -1_i);
        expect(combo.getSelectedOption().empty());
        combo.setOptions({"B", "C"});
        expect(combo.getSelectedOption() == "B");
        combo.setLabel("Other");
        combo.setOnChange({});
        combo.setEnabled(false);
        expect(combo.getLabel() == "Other" and !combo.isEnabled() and combo.getOptions().size() == 2_u);
        gui.frame(ui);

        const ComboBox empty("Empty", {});
        expect(empty.getSelected() == -1_i);
    };

    "RadioGroup: one option at a time, vertically or on one line"_test = [] {
        Gui gui;
        int picked = -1;
        RadioGroup radio("Quality##radio", {"Low", "High"}, 0, [&](int index, const std::string &) { picked = index; });
        const auto ui = [&] { inWindow([&] { radio.render(); }); };
        gui.frame(ui);
        activate(widget({"Quality##radio", 1, "High"}));
        gui.frame(ui);
        expect(picked == 1_i);
        expect(radio.getSelectedOption() == "High");

        radio.setHorizontal(true);
        radio.setSelected(9);
        expect(radio.getSelected() == 1_i);
        radio.setOptions({"Only"});
        expect(radio.getSelected() == 0_i);
        radio.setOptions({});
        expect(radio.getSelected() == -1_i and radio.getSelectedOption().empty());
        radio.setOptions({"X", "Y"});
        radio.setLabel("##hidden");
        radio.setOnChange({});
        radio.setEnabled(false);
        expect(radio.getLabel() == "##hidden" and !radio.isEnabled() and radio.getOptions().size() == 2_u);
        gui.frame(ui);
        const RadioGroup empty("Empty", {});
        expect(empty.getSelected() == -1_i);
    };

    "TextInput: typing changes the value, Enter submits it"_test = [] {
        Gui gui;
        std::vector<std::string> changes;
        std::string submitted;
        TextInput input("Name", "Enter a name", [&](const std::string &value) { changes.push_back(value); });
        input.setOnSubmit([&](const std::string &value) { submitted = value; });
        input.setWidth(200.0f);
        const auto ui = [&] { inWindow([&] { input.render(); }); };
        gui.frame(ui);
        activate(widget({"Name"}));
        gui.frame(ui);
        typeText("Ada");
        gui.frame(ui);
        expect(input.getValue() == "Ada");
        expect(changes == std::vector<std::string>{"Ada"});
        press(ImGuiKey_Enter);
        gui.frames(2, ui);
        expect(submitted == "Ada");

        input.setValue("x");
        input.setPassword(true);
        input.setPlaceholder("Secret");
        input.setLabel("Password");
        input.setOnChange({});
        input.setEnabled(false);
        expect(input.getLabel() == "Password" and !input.isEnabled());
        gui.frame(ui);
    };

    "NumberInput: +/- steps, typed values, clamped to its range"_test = [] {
        Gui gui;
        int lastAge = 0;
        NumberInput<int> age("Age", 18, 1, [&](int value) { lastAge = value; });
        age.setRange(0, 20);
        NumberInput<float> speed("Speed", 1.0f, 0.5f);
        NumberInput<double> ratio("Ratio", 0.5, 0.0);
        const auto ui = [&] {
            inWindow([&] {
                age.render();
                speed.render();
                ratio.render();
            });
        };
        gui.frame(ui);
        activate(widget({"Age", "+"}));
        gui.frame(ui);
        expect(age.getValue() == 19_i);
        expect(lastAge == 19_i);
        activate(widget({"Age", "+"}));
        gui.frame(ui);
        activate(widget({"Age", "+"}));
        gui.frame(ui);
        expect(age.getValue() == 20_i); // Clamped

        age.setValue(-5);
        expect(age.getValue() == 0_i);
        age.setStep(2);
        age.setWidth(120.0f);
        age.setLabel("Years");
        age.setOnChange({});
        age.setEnabled(false);
        expect(age.getLabel() == "Years" and !age.isEnabled());
        speed.setRange(0.0f, 2.0f);
        speed.setValue(3.0f);
        expect(speed.getValue() == 2.0_f);
        expect(ratio.getValue() == 0.5_d);
        gui.frame(ui);
    };
}

void run_dialog_tests()
{
    "confirm: Enter or the confirm button runs onConfirm, Escape or Cancel runs onCancel"_test = [] {
        Gui gui;
        DialogManager dialogs;
        std::vector<std::string> answers;
        const auto ask = [&](const std::string &title, bool destructive) {
            dialogs.confirm({.title = title,
                .message = "Sure?",
                .confirmLabel = "Yes",
                .destructive = destructive,
                .onConfirm = [&answers, title] { answers.push_back(title + ": yes"); },
                .onCancel = [&answers, title] { answers.push_back(title + ": no"); }});
        };
        const auto ui = [&] { dialogs.draw(); };
        gui.frame(ui);
        expect(!dialogs.isOpen());

        ask("First", true);
        ask("Second", false);
        expect(dialogs.pending() == 2_u);
        gui.frame(ui);
        press(ImGuiKey_Enter);
        gui.frames(3, ui);
        expect(answers == std::vector<std::string>{"First: yes"});

        press(ImGuiKey_Escape);
        gui.frames(3, ui);
        expect(answers.back() == "Second: no");
        expect(!dialogs.isOpen());
    };

    "the key press that opens a dialog does not answer it"_test = [] {
        Gui gui;
        DialogManager dialogs;
        int confirmed = 0;
        // As the console does: Enter runs "quit", which asks in the same frame.
        bool askNow = false;
        const auto ui = [&] {
            if (ImGui::IsKeyPressed(ImGuiKey_Enter) && !askNow) {
                askNow = true;
                dialogs.confirm({.title = "Quit", .onConfirm = [&] { confirmed++; }});
            }
            dialogs.draw();
        };
        gui.frame(ui);
        press(ImGuiKey_Enter);
        gui.frames(3, ui);
        expect(dialogs.isOpen());
        expect(confirmed == 0_i);
        press(ImGuiKey_Enter);
        gui.frames(3, ui);
        expect(confirmed == 1_i);
    };

    "the dialog's buttons answer too"_test = [] {
        Gui gui;
        DialogManager dialogs;
        int confirmed = 0;
        int cancelled = 0;
        const auto ui = [&] { dialogs.draw(); };
        const auto popup = [](const std::string &title, std::uint64_t id) {
            // A modal popup is a window named by its id string.
            return window(title + "##Dialog" + std::to_string(id));
        };

        dialogs.confirm({.title = "Click", .onConfirm = [&] { confirmed++; }, .onCancel = [&] { cancelled++; }});
        gui.frames(2, ui);
        expect(popup("Click", 1) != nullptr);
        activate(itemId(popup("Click", 1), {"Confirm"}));
        gui.frames(2, ui);
        expect(confirmed == 1_i);

        dialogs.confirm({.title = "Click", .onConfirm = [&] { confirmed++; }, .onCancel = [&] { cancelled++; }});
        gui.frames(2, ui);
        activate(itemId(popup("Click", 2), {"Cancel"}));
        gui.frames(2, ui);
        expect(cancelled == 1_i);
    };

    "alert: a single OK button; a callback may queue another dialog"_test = [] {
        Gui gui;
        DialogManager dialogs;
        int closed = 0;
        dialogs.alert("Info", "Done", [&] {
            closed++;
            dialogs.alert("Next", "Queued from the callback");
        });
        const auto ui = [&] { dialogs.draw(); };
        gui.frame(ui);
        press(ImGuiKey_KeypadEnter);
        gui.frames(3, ui);
        expect(closed == 1_i);
        expect(dialogs.pending() == 1_u);
        press(ImGuiKey_Escape);
        gui.frames(3, ui);
        expect(!dialogs.isOpen());
    };
}

void run_notification_tests()
{
    "notify: each level, its default duration, mirrored to a callback"_test = [] {
        Gui gui;
        NotificationManager notifications;
        std::vector<std::string> mirrored;
        notifications.setOnNotify([&](NotificationManager::Level, const std::string &title, const std::string &message) {
            mirrored.push_back(title + "|" + message);
        });
        const std::uint64_t info = notifications.info("Info");
        notifications.success("Success", "with a message");
        notifications.warning("Warning", "careful");
        notifications.error("Error", "broken");
        expect(notifications.count() == 4_u);
        expect(mirrored.front() == "Info|");
        gui.frames(2, [&] { notifications.draw(); });

        expect(notifications.dismiss(info));
        expect(!notifications.dismiss(info));
        notifications.clear();
        expect(notifications.count() == 0_u);
    };

    "notifications disappear after their duration; only the newest are shown"_test = [] {
        Gui gui;
        NotificationManager notifications;
        notifications.setMaxVisible(2);
        for (int i = 0; i < 4; i++)
            notifications.notify(NotificationManager::Level::Info, "Toast " + std::to_string(i), "", 0.05f);
        gui.frame([&] { notifications.draw(); });
        expect(window("##Notification1") != nullptr);
        expect(window("##Notification2") == nullptr);
        SDL_Delay(80);
        gui.frames(2, [&] { notifications.draw(); });
        expect(notifications.count() == 0_u);
    };

    "a click dismisses a notification; hovering it pauses its timer"_test = [] {
        Gui gui;
        NotificationManager notifications;
        notifications.notify(NotificationManager::Level::Warning, "Click me", "", 30.0f);
        const auto ui = [&] { notifications.draw(); };
        // An auto-sized window has its size after a couple of frames.
        gui.frames(3, ui);
        const ImGuiWindow *toast = window("##Notification0");
        expect(toast != nullptr);
        const ImVec2 center(toast->Pos.x + toast->Size.x * 0.5f, toast->Pos.y + toast->Size.y * 0.5f);
        mouseMove(center);
        gui.frames(2, ui);
        mouseButton(ImGuiMouseButton_Left, true);
        gui.frame(ui);
        mouseButton(ImGuiMouseButton_Left, false);
        gui.frames(2, ui);
        expect(notifications.count() == 0_u);
    };
}

void run_theme_tests()
{
    "the built-in themes: Dark by default, then Light; unknown names are refused"_test = [] {
        Gui gui;
        ThemeManager &themes = gui.themes();
        expect(themes.getThemeNames() == std::vector<std::string>{"Dark", "Light"});
        expect(themes.current().name == "Dark");
        expect(&ThemeManager::active() == &themes.current());
        std::string changed;
        themes.setOnChange([&](const Theme &theme) { changed = theme.name; });
        expect(themes.setTheme("Light"));
        expect(changed == "Light");
        expect(!themes.current().dark);
        expect(!themes.setTheme("Nope"));
        gui.frame([] { ImGui::Text("Light"); });
    };

    "add: a custom theme, applied at the display scale; names are unique"_test = [] {
        Gui gui;
        ThemeManager &themes = gui.themes();
        Theme custom = Theme::makeLight();
        custom.name = "Custom";
        themes.add(custom);
        expect(throws<std::invalid_argument>([&] { themes.add(custom); }));
        expect(themes.setTheme("Custom"));
        themes.setScale(2.0f);
        expect(themes.getScale() == 2.0_f);
        expect(ImGui::GetStyle().FontScaleDpi == 2.0_f);
        themes.setScale(1.0f);
    };

    "interactive shades: lighter on a dark theme, darker on a light one"_test = [] {
        const ImVec4 base(0.5f, 0.5f, 0.5f, 1.0f);
        const InteractiveColors dark = Theme::makeDark().interactive(base);
        const InteractiveColors light = Theme::makeLight().interactive(base);
        expect(dark.hovered.x > base.x);
        expect(light.hovered.x < base.x);
        expect(dark.base.x == base.x);
    };

    "without a ThemeManager, components get the Dark theme"_test = [] {
        expect(ThemeManager::active().name == "Dark");
        ThemeManager manager; // No ImGui context: applying does nothing
        expect(manager.setTheme("Light"));
        expect(ThemeManager::active().name == "Light");
    };

    "PerformanceOverlay: FPS colored by speed, anchored to a corner or the work area"_test = [] {
        Gui gui;
        PerformanceOverlay overlay(gui.timer());
        expect(overlay.isVisible());
        gui.frame([&] { overlay.render(); }); // No measurement yet
        for (int i = 0; i < 40 && gui.timer().getFps() == 0.0; i++) {
            SDL_Delay(20);
            gui.frame([&] {
                overlay.setAnchor(ImVec2(800, 100));
                overlay.render();
            });
        }
        expect(gui.timer().getFps() > 0.0_d);
        gui.frame([&] { overlay.render(); });
        overlay.setVisible(false);
        gui.frame([&] { overlay.render(); });
        expect(!overlay.isVisible());
    };
}
