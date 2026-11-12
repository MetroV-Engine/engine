#include <boost/ut.hpp>

#include "Support/Headless.hpp"
#include "core/CommandRegistry/CommandRegistry.hpp"
#include "core/EventManager/EventManager.hpp"
#include "core/FrameTimer/FrameTimer.hpp"
#include "core/Logger/Logger.hpp"
#include "core/PanelManager/PanelManager.hpp"

#include <SDL3/SDL.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

using namespace boost::ut;
using EditorTest::LogWatch;

void run_command_registry_tests()
{
    "split: words separated by spaces, double quotes group them"_test = [] {
        expect(CommandRegistry::split("theme  Light") == std::vector<std::string>{"theme", "Light"});
        expect(CommandRegistry::split(R"(echo "hello world" !)") == std::vector<std::string>{"echo", "hello world", "!"});
        // An empty quoted word is still a word; blanks alone are none.
        expect(CommandRegistry::split(R"(echo "")") == std::vector<std::string>{"echo", ""});
        expect(CommandRegistry::split("   ").empty());
    };

    "add: refuses empty names, spaces and duplicates"_test = [] {
        CommandRegistry commands;
        expect(throws<std::invalid_argument>([&] { commands.add("", "", "", {}); }));
        expect(throws<std::invalid_argument>([&] { commands.add("two words", "", "", {}); }));
        expect(throws<std::invalid_argument>([&] { commands.add("help", "", "", {}); }));
    };

    "execute: runs the handler with the arguments; unknown and failing commands are errors"_test = [] {
        CommandRegistry commands;
        std::vector<std::string> received;
        commands.add("say", "say <text>", "Say it", [&](const std::vector<std::string> &args) { received = args; });
        commands.add("fail", "fail", "Always fails", [](const auto &) { throw std::runtime_error("broken"); });
        LogWatch log;

        expect(commands.execute(R"(say "a b" c)"));
        expect(received == std::vector<std::string>{"a b", "c"});
        expect(!commands.execute(""));
        expect(!commands.execute("nope"));
        expect(log.contains(LogLevel::Error, "Unknown command \"nope\""));
        expect(!commands.execute("fail"));
        expect(log.contains(LogLevel::Error, "fail: broken (usage: fail)"));
    };

    "help and echo are built in"_test = [] {
        CommandRegistry commands;
        LogWatch log;
        expect(commands.execute("help"));
        expect(log.contains("echo <text>  -  Print a message"));
        expect(commands.execute(R"(echo "hello  there" you)"));
        expect(log.contains(LogLevel::Info, "hello  there you"));
    };

    "complete: the command names starting with a prefix, sorted"_test = [] {
        CommandRegistry commands;
        commands.add("height", "", "", [](const auto &) {});
        expect(commands.complete("he") == std::vector<std::string>{"height", "help"});
        expect(commands.complete("x").empty());
        expect(commands.commands().size() == 3_u);
    };
}

void run_logger_tests()
{
    Logger &logger = Logger::instance();

    "a message is stored with its level, category and source location"_test = [&] {
        LogWatch log;
        logger.warning("Physics", "Body has zero mass");
        std::vector<LogEntry> entries;
        logger.copyEntries(entries);
        const LogEntry &last = entries.back();
        expect(last.level == LogLevel::Warning);
        expect(last.category == "Physics");
        expect(last.file.ends_with("TestCore.cpp")) << last.file;
        expect(last.line > 0_i);
        expect(log.contains(LogLevel::Warning, "zero mass"));
    };

    "identical consecutive messages are merged with a counter"_test = [&] {
        const std::uint64_t before = logger.version();
        const std::size_t debugs = logger.count(LogLevel::Debug);
        logger.debug("Repeat", "same");
        logger.debug("Repeat", "same");
        std::vector<LogEntry> entries;
        logger.copyEntries(entries);
        expect(entries.back().count == 2_i);
        expect(logger.count(LogLevel::Debug) == debugs + 2);
        expect(logger.version() == before + 2);
    };

    "every level has a name and its own counter; clear resets the counters"_test = [&] {
        logger.info("A", "info");
        logger.success("A", "success");
        logger.error("A", "error");
        expect(std::string(Logger::levelName(LogLevel::Debug)) == "Debug");
        expect(std::string(Logger::levelName(LogLevel::Info)) == "Info");
        expect(std::string(Logger::levelName(LogLevel::Success)) == "Success");
        expect(std::string(Logger::levelName(LogLevel::Warning)) == "Warning");
        expect(std::string(Logger::levelName(LogLevel::Error)) == "Error");
        expect(logger.count(LogLevel::Success) > 0_u);
        logger.clear();
        expect(logger.count(LogLevel::Error) == 0_u);
        std::vector<LogEntry> entries;
        logger.copyEntries(entries);
        expect(entries.empty());
    };

    "categories are kept sorted, without duplicates"_test = [&] {
        logger.info("Zeta", "z");
        logger.info("Alpha", "a");
        logger.info("Alpha", "again");
        const std::vector<std::string> categories = logger.categories();
        expect(std::ranges::is_sorted(categories));
        expect(std::ranges::count(categories, "Alpha") == 1);
    };

    "the log is bounded: the oldest entries are dropped"_test = [&] {
        for (std::size_t i = 0; i < Logger::MaxEntries + 10; i++)
            logger.debug("Flood", std::to_string(i));
        std::vector<LogEntry> entries;
        logger.copyEntries(entries);
        expect(entries.size() == Logger::MaxEntries);
        expect(entries.back().message == std::to_string(Logger::MaxEntries + 9));
        logger.clear();
    };

    "a log file gets every message; an empty path stops it"_test = [&] {
        const std::string path = "logs/test/editor.log";
        expect(logger.setLogFile(path));
        logger.setTerminalLevel(LogLevel::Error);
        logger.warning("File", "written to the file");
        logger.info("Terminal", "below the terminal level");
        expect(logger.setLogFile(""));
        std::ifstream file(path);
        std::stringstream content;
        content << file.rdbuf();
        expect(content.str().find("[File] written to the file (") != std::string::npos) << content.str();
        expect(content.str().find("below the terminal level") != std::string::npos);
        // A directory cannot be opened as a file.
        expect(!logger.setLogFile("logs"));
        expect(logger.setLogFile(""));
    };

    "messages printed in the terminal down to its level"_test = [&] {
        logger.setTerminalLevel(LogLevel::Debug);
        logger.debug("Terminal", "printed on stdout");
        logger.warning("Terminal", "printed on stderr");
        logger.setTerminalLevel(LogLevel::Error);
    };

    "SDL's logs are captured, category SDL, with a level from their priority"_test = [&] {
        logger.captureSdlLogs();
        LogWatch log;
        SDL_SetLogPriorities(SDL_LOG_PRIORITY_TRACE);
        SDL_LogTrace(SDL_LOG_CATEGORY_APPLICATION, "sdl trace");
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "sdl warn");
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "sdl error");
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "sdl info");
        SDL_ResetLogPriorities();
        expect(log.contains(LogLevel::Debug, "sdl trace"));
        expect(log.contains(LogLevel::Warning, "sdl warn"));
        expect(log.contains(LogLevel::Error, "sdl error"));
        expect(log.contains(LogLevel::Info, "sdl info"));
        std::vector<LogEntry> entries;
        logger.copyEntries(entries);
        expect(entries.back().category == "SDL");
        expect(entries.back().file.empty());
    };

    "any thread can log"_test = [&] {
        LogWatch log;
        std::thread worker([&] {
            for (int i = 0; i < 100; i++)
                logger.info("Worker", "from a thread " + std::to_string(i));
        });
        for (int i = 0; i < 100; i++)
            logger.info("Main", "from main " + std::to_string(i));
        worker.join();
        expect(log.count("from a thread") == 100_u);
        expect(log.count("from main") == 100_u);
    };
}

void run_frame_timer_tests()
{
    "the first tick has no delta; then each tick measures the frame"_test = [] {
        FrameTimer timer;
        timer.tick();
        expect(timer.getDeltaTime() == 0.0_d);
        expect(timer.getFrameCount() == 1_ull);
        SDL_Delay(2);
        timer.tick();
        expect(timer.getDeltaTime() > 0.001_d);
        expect(timer.getHistory()[0] > 1.0_f);
        expect(timer.getHistoryOffset() == 1_u);
    };

    "FPS, average and max frame time are refreshed every half second"_test = [] {
        FrameTimer timer;
        timer.tick();
        expect(timer.getFps() == 0.0_d);
        while (timer.getFps() == 0.0) {
            SDL_Delay(20);
            timer.tick();
        }
        expect(timer.getFps() > 5.0_d and timer.getFps() < 60.0_d) << timer.getFps();
        expect(timer.getAverageFrameTimeMs() >= 19.0_d);
        expect(timer.getMaxFrameTimeMs() >= timer.getAverageFrameTimeMs());
    };

    "reset forgets the previous frame: no delta for the pause"_test = [] {
        FrameTimer timer;
        timer.tick();
        SDL_Delay(5);
        timer.reset();
        timer.tick();
        expect(timer.getDeltaTime() == 0.0_d);
    };
}

namespace {

SDL_Event userEvent(std::uint32_t type)
{
    SDL_Event event{};
    event.type = type;
    return event;
}

} // namespace

void run_event_manager_tests()
{
    // SDL's event queue needs SDL; the video subsystem is not needed.
    SDL_Init(SDL_INIT_EVENTS);

    "callbacks of every event first, then of the event's type"_test = [] {
        EventManager events;
        std::vector<std::string> calls;
        events.subscribeAll([&](const SDL_Event &) { calls.push_back("all"); });
        events.subscribe(SDL_EVENT_USER, [&](const SDL_Event &) { calls.push_back("user"); });
        SDL_Event user = userEvent(SDL_EVENT_USER);
        SDL_PushEvent(&user);
        SDL_Event other = userEvent(SDL_EVENT_USER + 1);
        SDL_PushEvent(&other);
        events.pollEvents();
        expect(calls == std::vector<std::string>{"all", "user", "all"});
    };

    "waitEvents dispatches what arrives, or returns after the timeout"_test = [] {
        EventManager events;
        int count = 0;
        events.subscribe(SDL_EVENT_USER, [&](const SDL_Event &) { count++; });
        events.waitEvents(1);
        expect(count == 0_i);
        for (int i = 0; i < 2; i++) {
            SDL_Event user = userEvent(SDL_EVENT_USER);
            SDL_PushEvent(&user);
        }
        events.waitEvents(100);
        expect(count == 2_i);
    };

    "logging: off, without motion, or everything (keys get their name)"_test = [] {
        EventManager events;
        expect(events.getLogging() == EventManager::Logging::Off);
        const auto push = [] {
            SDL_Event motion = userEvent(SDL_EVENT_MOUSE_MOTION);
            SDL_PushEvent(&motion);
            SDL_Event key = userEvent(SDL_EVENT_KEY_DOWN);
            key.key.key = SDLK_A;
            SDL_PushEvent(&key);
        };
        LogWatch log;
        push();
        events.pollEvents();
        expect(log.count("SDL_EVENT_") == 0_u);

        events.setLogging(EventManager::Logging::NoMotion);
        push();
        events.pollEvents();
        expect(log.contains(LogLevel::Debug, "key=\"A\""));
        expect(!log.contains("SDL_EVENT_MOUSE_MOTION"));

        events.setLogging(EventManager::Logging::All);
        push();
        events.pollEvents();
        expect(log.contains("SDL_EVENT_MOUSE_MOTION"));
    };

    SDL_Quit();
}

void run_gui_context_tests()
{
    using EditorTest::Gui;

    "a headless window, renderer and ImGui context; frames can be drawn"_test = [] {
        Gui gui;
        expect(gui.isRunning());
        expect(ImGui::GetCurrentContext() != nullptr);
        expect(std::string(gui.getRendererName()) == "software");
        expect(gui.getDisplayScale() > 0.0_f);
        gui.frames(3, [] { ImGui::Text("Hello"); });
        expect(gui.timer().getFrameCount() == 3_ull);
    };

    "the window state: normal size, maximized and fullscreen flags; restored clamped to the display"_test = [] {
        Gui gui;
        WindowState state = gui.getWindowState();
        expect(state.width == 1600_i and state.height == 900_i);
        expect(!state.maximized and !state.fullscreen);

        gui.setFullscreen(true);
        gui.frame();
        expect(gui.isFullscreen());
        gui.setFullscreen(false);
        gui.frame();
        expect(!gui.isFullscreen());

        WindowState saved;
        saved.width = 100; // below the minimum
        saved.height = 100;
        saved.hasPosition = true;
        saved.x = 10;
        saved.y = 20;
        gui.applyWindowState(saved);
        gui.frame();
        state = gui.getWindowState();
        expect(state.width == 640_i and state.height == 400_i) << state.width << state.height;

        saved.maximized = true;
        saved.fullscreen = true;
        gui.applyWindowState(saved);
        gui.frame();
        expect(gui.isFullscreen());
        gui.setFullscreen(false);
        gui.setWindowSize(1280, 720);
        gui.frame();
    };

    "VSync can be switched on and off"_test = [] {
        Gui gui;
        gui.setVSync(true);
        gui.setVSync(false);
        expect(!gui.isVSync());
    };

    "a close request goes to the handler if any, otherwise stops"_test = [] {
        Gui gui;
        int requests = 0;
        gui.setCloseRequestHandler([&] { requests++; });
        SDL_Event close{};
        close.type = SDL_EVENT_WINDOW_CLOSE_REQUESTED;
        SDL_PushEvent(&close);
        gui.processEvents();
        expect(requests == 1_i);
        expect(gui.isRunning());
        gui.setCloseRequestHandler({});
        SDL_PushEvent(&close);
        gui.processEvents();
        expect(!gui.isRunning());
    };

    // One GuiContext at a time: ImGui has one current context.
    "SDL_EVENT_QUIT always stops"_test = [] {
        Gui other;
        SDL_Event quit{};
        quit.type = SDL_EVENT_QUIT;
        SDL_PushEvent(&quit);
        other.run([] {});
        expect(!other.isRunning());
    };

    "run: draws frames until stopped; minimized, it sleeps until the window comes back"_test = [] {
        Gui gui;
        int frames = 0;
        SDL_TimerID restore = 0;
        gui.run([&] {
            frames++;
            if (frames == 2) {
                // Hidden: no frame is drawn until an event shows it again.
                SDL_Event minimized{};
                minimized.type = SDL_EVENT_WINDOW_MINIMIZED;
                SDL_PushEvent(&minimized);
                restore = SDL_AddTimer(
                    150,
                    [](void *, SDL_TimerID, Uint32) -> Uint32 {
                        SDL_Event restored{};
                        restored.type = SDL_EVENT_WINDOW_RESTORED;
                        SDL_PushEvent(&restored);
                        return 0;
                    },
                    nullptr);
            }
            if (frames == 4)
                gui.stop();
        });
        expect(frames == 4_i);
        expect(restore != 0u);
    };

    "run: a quit while minimized ends the loop"_test = [] {
        int frames = 0;
        Gui again;
        again.run([&] {
            frames++;
            if (frames == 1) {
                SDL_Event minimized{};
                minimized.type = SDL_EVENT_WINDOW_MINIMIZED;
                SDL_PushEvent(&minimized);
                SDL_Event quit{};
                quit.type = SDL_EVENT_QUIT;
                SDL_PushEvent(&quit);
            }
        });
        expect(frames == 1_i);
    };

    "window events: resize and move are remembered, a scale change rescales the theme"_test = [] {
        Gui gui;
        for (const SDL_EventType type : {SDL_EVENT_WINDOW_RESIZED, SDL_EVENT_WINDOW_MOVED, SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED,
                 SDL_EVENT_WINDOW_HIDDEN, SDL_EVENT_WINDOW_SHOWN}) {
            SDL_Event event{};
            event.type = type;
            SDL_PushEvent(&event);
        }
        gui.frame();
        expect(gui.getWindowState().width > 0_i);
    };

    "a slow frame is reported"_test = [] {
        Gui gui;
        LogWatch log;
        gui.frame();
        SDL_Delay(120);
        gui.frame();
        expect(log.contains(LogLevel::Warning, "Slow frame"));
    };

    "a window that cannot be created throws and leaves nothing behind"_test = [] {
        SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
        SDL_SetHint(SDL_HINT_RENDER_DRIVER, "no-such-renderer");
        expect(throws<std::runtime_error>([] { GuiContext gui("Broken", 640, 480); }));
        expect(ImGui::GetCurrentContext() == nullptr);

        SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "no-such-driver");
        expect(throws<std::runtime_error>([] { GuiContext gui("Broken", 640, 480); }));
    };
}

void run_panel_manager_tests()
{
    using EditorTest::Gui;

    "panels are registered once, found by name, shown, hidden and removed"_test = [] {
        Gui gui;
        PanelManager panels;
        int drawn = 0;
        Panel &first = panels.registerPanel("First", [&] { drawn++; });
        panels.registerPanel("Second", {});
        expect(throws<std::invalid_argument>([&] { panels.registerPanel("First", [] {}); }));
        expect(panels.getPanel("First") == &first);
        expect(panels.getPanel("Nope") == nullptr);
        expect(panels.hasPanel("Second"));

        gui.frame([&] { panels.drawAll(); });
        expect(drawn == 1_i);
        expect(panels.toggle("First"));
        expect(!first.isVisible());
        gui.frame([&] { panels.drawAll(); });
        expect(drawn == 1_i);
        expect(panels.setVisible("First", true));
        gui.frame([&] { panels.drawAll(); });
        expect(drawn == 2_i);

        expect(!panels.toggle("Nope"));
        expect(!panels.setVisible("Nope", true));
        expect(panels.unregisterPanel("Second"));
        expect(!panels.unregisterPanel("Second"));
        expect(panels.getPanels().size() == 1_u);
    };

    "a panel: initial size, minimum size, padding, no close button"_test = [] {
        struct Sized : Panel
        {
            Sized() : Panel("Sized") {}
            int drawn = 0;
            void drawContent() override { drawn++; }
            ImVec2 minimumSize() const override { return ImVec2(300.0f, 200.0f); }
        };
        Gui gui;
        PanelManager panels;
        Sized &panel = panels.registerPanel<Sized>();
        panel.setInitialSize(ImVec2(400.0f, 300.0f));
        panel.setWindowPadding(ImVec2(0.0f, 0.0f));
        panel.setClosable(false);
        expect(!panel.isClosable());
        gui.frames(2, [&] { panels.drawAll(); });
        expect(panel.drawn == 2_i);
        const ImGuiWindow *window = EditorTest::window("Sized");
        expect(window != nullptr);
        expect(window->Size.x >= 300.0_f and window->Size.y >= 200.0_f);
    };
}
