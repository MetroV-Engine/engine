#include <boost/ut.hpp>

#include "Support/Headless.hpp"
#include "core/CommandRegistry/CommandRegistry.hpp"
#include "panels/ConsolePanel/ConsolePanel.hpp"

#include <string>

using namespace boost::ut;
using namespace EditorTest;

namespace {

struct ConsoleFixture
{
    Gui gui;
    CommandRegistry commands;
    ConsolePanel panel{commands};
    ImVec2 size{900, 500};

    void frame(int count = 1)
    {
        gui.frames(count, [this] {
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(size);
            panel.draw();
        });
    }
    ImGuiWindow *console() const { return window("Console"); }
    // Types a command in the command line and runs it.
    void run(const std::string &line)
    {
        typeText(line);
        frame();
        press(ImGuiKey_Enter);
        frame(3);
    }
    ImGuiWindow *openMenu(ImGuiID popupId)
    {
        openPopup(popupId);
        frame(2);
        return popupWindow(popupId);
    }
    std::string levelLabel(LogLevel level) const
    {
        const std::string name = Logger::levelName(level);
        return name + " " + std::to_string(Logger::instance().count(level)) + "##Level" + name;
    }
};

} // namespace

void run_console_panel_tests()
{
    "commands typed in the command line run; history with Up/Down; Tab completes"_test = [] {
        ConsoleFixture f;
        Logger::instance().clear();
        f.frame(2); // "No messages."
        f.panel.focusCommandLine();
        f.frame(2);
        LogWatch log;
        f.run("echo first");
        f.run("echo second");
        f.run("echo second"); // Not added twice to the history
        f.run("   ");          // Blank: nothing runs
        expect(log.count(LogLevel::Info, "first") == 1_u);
        expect(log.count(LogLevel::Info, "second") == 2_u);

        press(ImGuiKey_UpArrow);
        f.frame(2);
        press(ImGuiKey_UpArrow);
        f.frame(2);
        press(ImGuiKey_UpArrow); // Stays on the oldest
        f.frame(2);
        press(ImGuiKey_DownArrow);
        f.frame(2);
        press(ImGuiKey_DownArrow); // Back to an empty line
        f.frame(2);
        press(ImGuiKey_DownArrow);
        f.frame(2);
        press(ImGuiKey_UpArrow);
        f.frame(2);
        press(ImGuiKey_Enter); // Runs "echo second" again
        f.frame(3);
        expect(log.count(LogLevel::Info, "second") == 3_u);

        // Tab: one match completes it, several complete their common prefix and are listed.
        f.commands.add("height", "height", "", [](const auto &) {});
        typeText("ec");
        f.frame();
        press(ImGuiKey_Tab);
        f.frame(2);
        typeText("done");
        f.frame();
        press(ImGuiKey_Enter);
        f.frame(3);
        expect(log.contains("done"));
        typeText("he");
        f.frame();
        press(ImGuiKey_Tab);
        f.frame(2);
        expect(log.contains("height  help"));
        typeText("lp x");
        f.frame();
        press(ImGuiKey_Tab); // After a space: nothing to complete
        f.frame(2);
        press(ImGuiKey_Escape); // Clears the line
        f.frame(2);
        typeText("zz");
        f.frame();
        press(ImGuiKey_Tab); // No match
        f.frame(2);
        press(ImGuiKey_Escape);
        f.frame(2);
    };

    "filters: levels, categories, text; Clear and Copy"_test = [] {
        ConsoleFixture f;
        Logger &logger = Logger::instance();
        logger.clear();
        logger.debug("Physics", "debug line");
        logger.info("Physics", "info line");
        logger.success("Game", "success line");
        logger.warning("Game", "warning line");
        logger.error("Renderer", "error line");
        logger.error("Renderer", "error line"); // x2
        f.frame(3);
        expect(f.panel.isLevelVisible(LogLevel::Info));
        expect(!f.panel.isLevelVisible(LogLevel::Debug));

        activate(itemId(f.console(), {f.levelLabel(LogLevel::Debug)}));
        f.frame(2);
        expect(f.panel.isLevelVisible(LogLevel::Debug));
        activate(itemId(f.console(), {f.levelLabel(LogLevel::Info)}));
        f.frame(2);
        expect(!f.panel.isLevelVisible(LogLevel::Info));

        // Categories: unchecked in the dropdown.
        activate(itemId(f.console(), {"##Categories"}));
        f.frame(2);
        ImGuiWindow *combo = window("##Combo_00");
        expect(combo != nullptr);
        activate(itemId(combo, {"Game"}));
        f.frame(2);
        expect(f.panel.getHiddenCategories().contains("Game"));
        activate(itemId(combo, {"Game"}));
        f.frame(2);
        expect(!f.panel.getHiddenCategories().contains("Game"));
        activate(itemId(combo, {"Renderer"}));
        f.frame(2);
        press(ImGuiKey_Escape);
        f.frame(2);
        f.panel.setHiddenCategories({});

        // Text filter, matching the message or the category.
        activate(itemId(f.console(), {"##Filter"}));
        f.frame(2);
        typeText("physics");
        f.frame(2);
        press(ImGuiKey_Escape);
        f.frame(2);
        activate(itemId(f.console(), {"##Filter"}));
        f.frame(2);
        typeText("nothing matches this");
        f.frame(2);
        press(ImGuiKey_Escape);
        f.frame(2);

        activate(itemId(f.console(), {"Auto-scroll"}));
        f.frame(2);
        expect(!f.panel.isAutoScroll());
        f.panel.setAutoScroll(true);
        f.panel.setLevelVisible(LogLevel::Info, true);
        activate(itemId(f.console(), {"Copy"}));
        f.frame(2);
        activate(itemId(f.console(), {"Clear"}));
        f.frame(2);
        expect(Logger::instance().count(LogLevel::Error) == 0_u);
    };

    "a message's menu copies it or hides its category"_test = [] {
        ConsoleFixture f;
        Logger &logger = Logger::instance();
        logger.clear();
        logger.warning("Menus", "with a source location");
        logger.log(LogLevel::Info, "External", "without one", std::source_location());
        f.frame(3);
        std::vector<LogEntry> entries;
        logger.copyEntries(entries);
        const ImGuiID table = itemId(window("Console/##Log_"), {"##LogTable"});
        for (const LogEntry &entry : entries) {
            const ImGuiID menuId = itemId(table, {static_cast<int>(entry.id), "##EntryMenu"});
            for (const char *item : {"Copy message", "Copy line", "Copy source location"}) {
                ImGuiWindow *menu = f.openMenu(menuId);
                expect(menu != nullptr) << entry.message;
                if (!menu)
                    break;
                activate(itemId(menu, {item}));
                f.frame(2);
            }
        }
        const LogEntry &first = entries.front();
        ImGuiWindow *menu = f.openMenu(itemId(table, {static_cast<int>(first.id), "##EntryMenu"}));
        activate(itemId(menu, {"Hide category \"Menus\""}));
        f.frame(2);
        expect(f.panel.getHiddenCategories().contains("Menus"));
    };

    "a narrow console puts its filters on a second line; many messages are clipped"_test = [] {
        ConsoleFixture f;
        f.size = ImVec2(300, 300);
        for (int i = 0; i < 300; i++)
            Logger::instance().info("Flood", "message " + std::to_string(i));
        f.frame(3);
        f.panel.setAutoScroll(false);
        Logger::instance().info("Flood", "one more");
        f.frame(2);
    };
}
