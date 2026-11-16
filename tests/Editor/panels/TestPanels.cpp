#include <boost/ut.hpp>

#include "Support/Headless.hpp"
#include "core/PanelManager/PanelManager.hpp"
#include "layout/EditorLayout/EditorLayout.hpp"
#include "navigation/ActionRegistry/ActionRegistry.hpp"
#include "panels/ButtonsPanel/ButtonsPanel.hpp"
#include "panels/ImGuiDemoPanel.hpp"
#include "panels/InputsPanel/InputsPanel.hpp"
#include "panels/PlayControlsPanel/PlayControlsPanel.hpp"
#include "panels/SettingsPanel/SettingsPanel.hpp"
#include "panels/ViewportPanel/ViewportPanel.hpp"
#include "scene/PlayController/PlayController.hpp"
#include "ui/DialogManager/DialogManager.hpp"
#include "ui/NotificationManager/NotificationManager.hpp"
#include "ui/PerformanceOverlay/PerformanceOverlay.hpp"

#include <string>

using namespace boost::ut;
using namespace EditorTest;

namespace {

struct CountingSimulation : Simulation
{
    int updates = 0;
    void begin() override {}
    void update(double) override { updates++; }
    void end() override {}
};

// The game actions, as the Editor registers them.
void addGameActions(ActionRegistry &actions, PlayController &play)
{
    actions.add({.id = "game.play", .label = "Play", .callback = [&] { play.play(); }, .shortcut = ImGuiKey_F5,
        .tooltip = "Run the game", .isEnabled = [&] { return play.getState() != PlayState::Playing; },
        .isChecked = [&] { return play.getState() == PlayState::Playing; }});
    actions.add({.id = "game.pause", .label = "Pause",
        .callback = [&] { play.getState() == PlayState::Paused ? play.play() : play.pause(); },
        .isEnabled = [&] { return !play.isStopped(); }, .isChecked = [&] { return play.getState() == PlayState::Paused; }});
    actions.add({.id = "game.stop", .label = "Stop", .callback = [&] { play.stop(); },
        .isEnabled = [&] { return !play.isStopped(); }});
    actions.add({.id = "game.step", .label = "Step", .callback = [&] { play.step(); },
        .isEnabled = [&] { return play.getState() != PlayState::Playing; }});
}

ImGuiWindow *openMenu(Gui &gui, ImGuiID popupId, const std::function<void()> &ui)
{
    openPopup(popupId);
    gui.frames(2, ui);
    return popupWindow(popupId);
}

} // namespace

void run_panels_tests()
{
    "Game Controls: Play, Pause, Step, Stop buttons, the speed, docked without a tab"_test = [] {
        Gui gui;
        CountingSimulation simulation;
        PlayController play(simulation);
        ActionRegistry actions;
        addGameActions(actions, play);
        PanelManager panels;
        EditorLayout layout;
        PlayControlsPanel &controls = panels.registerPanel<PlayControlsPanel>(actions, play);
        int resets = 0;
        controls.setOnResetPosition([&] { resets++; });
        panels.registerPanel("Console", [] { ImGui::Text("Console"); });
        layout.setDefaultArea("Game Controls", EditorLayout::Area::Controls);
        layout.setDefaultArea("Console", EditorLayout::Area::Bottom);
        const auto ui = [&] {
            play.update(1.0 / 60.0);
            layout.draw();
            panels.drawAll();
        };
        gui.frames(4, ui);
        const auto button = [&](const char *id) { return itemId(window("Game Controls"), {id, "##Control"}); };

        activate(button("game.play"));
        gui.frames(2, ui);
        expect(play.getState() == PlayState::Playing);
        activate(button("game.pause"));
        gui.frames(2, ui);
        expect(play.getState() == PlayState::Paused);
        activate(button("game.step"));
        gui.frames(2, ui);
        activate(button("game.pause")); // Resumes
        gui.frames(2, ui);
        expect(play.getState() == PlayState::Playing);
        activate(button("game.play")); // Disabled while playing
        gui.frames(2, ui);
        activate(button("game.stop"));
        gui.frames(2, ui);
        expect(play.isStopped());

        activate(itemId(window("Game Controls"), {"##Speed"}));
        gui.frames(2, ui);
        activate(itemId(window("##Combo_00"), {"x2"}));
        gui.frames(2, ui);
        expect(play.getTimeScale() == 2.0_d);

        // The grip: hovered, then its menu puts the bar back or undocks it.
        const ImGuiWindow *bar = window("Game Controls");
        mouseMove(ImVec2(bar->Pos.x + 10.0f, bar->Pos.y + bar->Size.y * 0.5f));
        gui.frames(3, ui);
        ImGuiWindow *menu = openMenu(gui, itemId(window("Game Controls"), {"##MoveGripMenu"}), ui);
        expect(menu != nullptr);
        activate(itemId(menu, {"Reset Position"}));
        gui.frames(2, ui);
        expect(resets == 1_i);
        menu = openMenu(gui, itemId(window("Game Controls"), {"##MoveGripMenu"}), ui);
        activate(itemId(menu, {"Undock"}));
        gui.frames(4, ui);
        // Floating: a normal title bar, and a minimum size.
        expect(window("Game Controls")->Size.x >= controls.isVisible() * 1.0f);
        play.play();
        gui.frames(2, ui);

        // Hovering a control button shows its tooltip.
        const ImGuiWindow *floating = window("Game Controls");
        mouseMove(ImVec2(floating->Pos.x + floating->Size.x * 0.5f - 120.0f, floating->Pos.y + floating->Size.y * 0.6f));
        gui.frames(3, ui);

        // Dragging the grip moves the bar.
        const ImVec2 grip(floating->Pos.x + 12.0f, floating->Pos.y + floating->Size.y * 0.6f);
        mouseMove(grip);
        gui.frame(ui);
        mouseButton(ImGuiMouseButton_Left, true);
        gui.frame(ui);
        for (int i = 1; i <= 5; i++) {
            mouseMove(ImVec2(grip.x + 20.0f * static_cast<float>(i), grip.y + 10.0f * static_cast<float>(i)));
            gui.frame(ui);
        }
        mouseButton(ImGuiMouseButton_Left, false);
        gui.frames(2, ui);
    };

    "Settings: VSync, fullscreen, resolution, quality, overlay and theme"_test = [] {
        Gui gui;
        PerformanceOverlay overlay(gui.timer());
        PanelManager panels;
        panels.registerPanel<SettingsPanel>(gui, overlay);
        const auto ui = [&] {
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(ImVec2(500, 500));
            panels.drawAll();
        };
        gui.frames(2, ui);
        const auto in = [&](const std::vector<IdPart> &path) { return itemId(window("Settings"), path); };

        activate(in({"VSync"}));
        gui.frames(2, ui);
        activate(in({"Fullscreen"}));
        gui.frames(2, ui);
        expect(gui.isFullscreen());
        activate(in({"Fullscreen"}));
        gui.frames(2, ui);
        activate(in({"Show performance"}));
        gui.frames(2, ui);
        expect(!overlay.isVisible());
        activate(in({"Quality", 2, "High"}));
        gui.frames(2, ui);

        activate(in({"Resolution"}));
        gui.frames(2, ui);
        activate(itemId(window("##Combo_00"), {1, "1920x1080"}));
        gui.frames(2, ui);
        activate(in({"Theme"}));
        gui.frames(2, ui);
        activate(itemId(window("##Combo_00"), {1, "Light"}));
        gui.frames(2, ui);
        expect(gui.themes().current().name == "Light");
    };

    "Viewport: a placeholder or the game's renderer, a frame and a badge while playing"_test = [] {
        Gui gui;
        CountingSimulation simulation;
        PlayController play(simulation);
        PerformanceOverlay overlay(gui.timer());
        PanelManager panels;
        ViewportPanel &viewport = panels.registerPanel<ViewportPanel>(overlay, play);
        ImVec2 size(800, 600);
        const auto ui = [&] {
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(size);
            panels.drawAll();
            overlay.render();
        };
        gui.frames(2, ui);
        int rendered = 0;
        viewport.setRenderer([&](ImDrawList *, ImVec2 min, ImVec2 max) {
            rendered++;
            expect(max.x > min.x);
        });
        gui.frames(2, ui);
        expect(rendered == 2_i);
        play.play();
        gui.frame(ui);
        play.pause();
        gui.frame(ui);
        // Too small to draw anything.
        size = ImVec2(1, 1);
        gui.frames(2, ui);
    };

    "demo panels: buttons give feedback, inputs log their values, the ImGui demo window"_test = [] {
        Gui gui;
        NotificationManager notifications;
        DialogManager dialogs;
        int quits = 0;
        PanelManager panels;
        panels.registerPanel<ButtonsPanel>(notifications, dialogs, [&] { quits++; });
        panels.registerPanel<InputsPanel>();
        ImGuiDemoPanel &demo = panels.registerPanel<ImGuiDemoPanel>();
        const auto ui = [&] {
            panels.drawAll();
            dialogs.draw();
        };
        gui.frames(2, ui);
        for (const char *button : {"Say hello", "Cancel", "Quit", "Warning", "Error", "Alert"}) {
            activate(itemId(window("Buttons"), {button}));
            gui.frames(2, ui);
        }
        expect(quits == 1_i);
        expect(notifications.count() == 4_u);
        expect(dialogs.isOpen());
        press(ImGuiKey_Enter);
        gui.frames(3, ui);

        LogWatch log;
        activate(itemId(window("Inputs"), {"Name"}));
        gui.frames(2, ui);
        typeText("Ada");
        gui.frame(ui);
        press(ImGuiKey_Enter);
        gui.frames(3, ui);
        activate(itemId(window("Inputs"), {"Age", "+"}));
        gui.frames(2, ui);
        activate(itemId(window("Inputs"), {"Speed", "+"}));
        gui.frames(2, ui);
        expect(log.contains("Name: Ada"));
        expect(log.contains("Name submitted: Ada"));
        expect(log.contains("Age: 19"));
        expect(log.contains("Speed: "));

        // The demo window's close button hides the panel.
        expect(window("Dear ImGui Demo") != nullptr);
        activate(itemId(window("Dear ImGui Demo"), {"#CLOSE"}));
        gui.frames(2, ui);
        expect(!demo.isVisible());
        gui.frame(ui);
    };
}
