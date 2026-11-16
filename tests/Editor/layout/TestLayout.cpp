#include <boost/ut.hpp>

#include "Support/Headless.hpp"
#include "layout/EditorLayout/EditorLayout.hpp"
#include "layout/StatusBar/StatusBar.hpp"
#include "layout/Workspace/Workspace.hpp"

#include <filesystem>
#include <fstream>
#include <string>

using namespace boost::ut;
using namespace EditorTest;

namespace {

std::string readFile(const std::string &path)
{
    std::ifstream file(path);
    return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

// Panels docked in the default layout.
void drawPanels(const char *const *names, int count)
{
    for (int i = 0; i < count; i++) {
        ImGui::Begin(names[i]);
        ImGui::Text("%s", names[i]);
        ImGui::End();
    }
}

} // namespace

void run_layout_tests()
{
    "EditorLayout: the default layout docks every panel in its area"_test = [] {
        Gui gui;
        EditorLayout layout;
        expect((ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DockingEnable) != 0);
        const char *names[] = {"Left", "Center", "Right", "Bottom", "Controls", "Second Left"};
        layout.setDefaultArea("Left", EditorLayout::Area::Left);
        layout.setDefaultArea("Center", EditorLayout::Area::Center);
        layout.setDefaultArea("Right", EditorLayout::Area::Center);
        layout.setDefaultArea("Right", EditorLayout::Area::Right); // Moved
        layout.setDefaultArea("Bottom", EditorLayout::Area::Bottom);
        layout.setDefaultArea("Controls", EditorLayout::Area::Controls);
        layout.setDefaultArea("Second Left", EditorLayout::Area::Left);
        layout.setAreaRatios(0.2f, 0.25f, 0.3f);
        const auto ui = [&] {
            layout.draw();
            drawPanels(names, 6);
        };
        gui.frames(4, ui);
        for (const char *name : names) {
            const ImGuiWindow *panel = window(name);
            expect(panel != nullptr and panel->DockNode != nullptr) << name;
        }
        // Sharing the left area: tabs, the first one declared selected.
        expect(window("Left")->DockNode == window("Second Left")->DockNode);
        expect(window("Left")->DockNode->SelectedTabId == window("Left")->TabId);
        // Left of center, right of it.
        expect(window("Left")->Pos.x < window("Center")->Pos.x);
        expect(window("Right")->Pos.x > window("Center")->Pos.x);

        layout.resetLayout();
        gui.frames(4, ui);
        expect(window("Left")->DockNode != nullptr);
    };

    "EditorLayout: without a controls strip, the bottom area is not split"_test = [] {
        Gui gui;
        EditorLayout layout;
        layout.setDefaultArea("Only", EditorLayout::Area::Bottom);
        layout.setDefaultArea("Missing", EditorLayout::Area::Left); // Never drawn
        const char *names[] = {"Only"};
        gui.frames(4, [&] {
            layout.draw();
            drawPanels(names, 1);
        });
        expect(window("Only")->DockNode != nullptr);
    };

    "StatusBar: a message on the left, items on the right; hidden, it takes no space"_test = [] {
        Gui gui;
        StatusBar bar;
        expect(bar.getMessage() == "Ready");
        bar.setMessage("Saved");
        expect(bar.getMessage() == "Saved");
        int drawn = 0;
        const auto ui = [&] { bar.draw(); };
        gui.frame(ui);
        expect(window("##StatusBar") != nullptr);
        bar.addItem([&] {
            drawn++;
            ImGui::Text("60 FPS");
        });
        bar.addItem([] { ImGui::Text("Dark"); });
        gui.frames(2, ui);
        expect(drawn == 2_i);
        bar.setVisible(false);
        expect(!bar.isVisible());
        gui.frame(ui);
        expect(drawn == 2_i);
    };
}

void run_workspace_tests()
{
    "values: typed helpers with fallbacks"_test = [] {
        Workspace::Values values;
        Workspace::setBool(values, "on", true);
        Workspace::setBool(values, "off", false);
        Workspace::setInt(values, "width", 1280);
        values["word"] = "true";
        values["bad"] = "12px";
        values["empty"] = "";
        expect(Workspace::getBool(values, "on", false));
        expect(!Workspace::getBool(values, "off", true));
        expect(Workspace::getBool(values, "word", false));
        expect(Workspace::getBool(values, "missing", true));
        expect(Workspace::getInt(values, "width", 0) == 1280_i);
        expect(Workspace::getInt(values, "bad", 7) == 7_i);
        expect(Workspace::getInt(values, "empty", 7) == 7_i);
        expect(Workspace::getInt(values, "missing", 3) == 3_i);
        expect(Workspace::getString(values, "word", "") == "true");
        expect(Workspace::getString(values, "missing", "fallback") == "fallback");
    };

    "saved in the ini file next to the layout, and restored on the next launch"_test = [] {
        std::filesystem::remove("workspace-test.ini");
        int width = 800;
        {
            Gui gui;
            ImGui::GetIO().IniFilename = "workspace-test.ini";
            Workspace workspace([&](Workspace::Values &values) { Workspace::setInt(values, "width", width); },
                [](const Workspace::Values &) {});
            expect(!workspace.load()); // Nothing saved yet
            gui.frame([&] { workspace.update(); });
            width = 1024;
            // Checked every half second.
            SDL_Delay(520);
            gui.frame([&] { workspace.update(); });
            workspace.save();
            expect(readFile("workspace-test.ini").find("[Workspace][Editor]\nwidth=1024") != std::string::npos);
            gui.frame([&] { workspace.update(); }); // Unchanged
        } // Saved again when destroyed

        Gui gui;
        ImGui::GetIO().IniFilename = "workspace-test.ini";
        int restored = 0;
        Workspace workspace([](Workspace::Values &) {},
            [&](const Workspace::Values &values) { restored = Workspace::getInt(values, "width", 0); });
        LogWatch log;
        expect(workspace.load());
        expect(restored == 1024_i);
        expect(log.contains("Workspace restored from workspace-test.ini"));
    };

    "other sections of the ini file are left to ImGui"_test = [] {
        std::ofstream("workspace-other.ini") << "[Workspace][Other]\nwidth=5\n\n[Workspace][Editor]\nnoequal\nheight=7\n";
        Gui gui;
        ImGui::GetIO().IniFilename = "workspace-other.ini";
        int height = 0;
        int width = 0;
        Workspace workspace([](Workspace::Values &) {}, [&](const Workspace::Values &values) {
            height = Workspace::getInt(values, "height", 0);
            width = Workspace::getInt(values, "width", 0);
        });
        expect(workspace.load());
        expect(height == 7_i and width == 0_i);
    };

    "without an ini file nothing is read or written; destroyed after ImGui, it does nothing"_test = [] {
        auto gui = std::make_unique<Gui>();
        auto workspace = std::make_unique<Workspace>([](Workspace::Values &) {}, [](const Workspace::Values &) {});
        expect(!workspace->load());
        workspace->save();
        gui.reset();
        workspace.reset();
    };
}
