// The runtime: runs a scene file with the engine, without the editor.
//
//   runtime <scene file> [--fps N]
//
// It installs every game linked into the program (GameCatalog: their
// services), builds a DataScene from the file (entities + the systems it
// lists), pushes it and runs the engine's loop until Ctrl+C or until no scene
// is left. It names no game: which games it contains is chosen in CMake.
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <string>

#include "Engine/Engine.hpp"
#include "Engine/Game/GameCatalog.hpp"
#include "Engine/Scenes/DataScene.hpp"

namespace {
    // TEMPORARY: the whole Ctrl+C handling below (the flag, the signal handler
    // and StopOnInterruptSystem, added to every scene by the runtime's setup).
    //
    // It exists because the runtime has no other way to end a game: no window
    // to close, and no way for a game to ask the engine to quit. Remove it, and
    // the std::signal call and the setup in main(), once both are true:
    //  1. the engine ends its loop itself when the program is asked to quit
    //     (window closed, Ctrl+C / SIGINT), through its platform layer (the
    //     window and input services, issues 8 and 9 of EDITOR_ENGINE_PLAN.md);
    //  2. a game can ask the engine to stop from a system, through a service
    //     (scene requests, #111, or a dedicated quit request).
    // Until then, Ctrl+C is the only way out.

    // Set by Ctrl+C. A signal handler may only touch such a flag: the engine
    // is stopped from a system, between two frames.
    volatile std::sig_atomic_t interrupted = 0;

    extern "C" void onInterrupt(int) { interrupted = 1; }

    // Added by the runtime to every scene it runs (not written in the file):
    // stops the engine after the frame in which Ctrl+C was pressed.
    class StopOnInterruptSystem : public ECS::ISystem {
        public:
            explicit StopOnInterruptSystem(Engine& engine) : _engine(engine) {}
            void update(ECS::Registry&, double) override {
                if (interrupted)
                    _engine.stop();
            }
            const std::string name() const override { return "runtime::StopOnInterruptSystem"; }

        private:
            Engine& _engine;
    };

    void printUsage(const char* program) {
        std::printf("Usage: %s <scene file> [--fps N]\n\n"
                    "Runs a scene file with the engine. Ctrl+C stops it.\n\n"
                    "  --fps N      Frames per second (default 60, 0: as fast as possible)\n"
                    "  --help, -h   Show this help\n",
                    program);
    }
}

int main(int argc, char** argv)
{
    std::string file;
    double fps = 60.0;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 0;
        }
        if (arg == "--fps" && i + 1 < argc) {
            char* end = nullptr;
            fps = std::strtod(argv[++i], &end);
            if (*end != '\0' || fps < 0.0) {
                std::fprintf(stderr, "Invalid --fps value: %s\n", argv[i]);
                return 2;
            }
        } else if (file.empty() && arg.rfind("--", 0) != 0) {
            file = arg;
        } else {
            std::fprintf(stderr, "Unknown argument: %s\n\n", arg.c_str());
            printUsage(argv[0]);
            return 2;
        }
    }
    if (file.empty()) {
        printUsage(argv[0]);
        return 2;
    }

    // TEMPORARY (see the comment at the top): Ctrl+C handling.
    std::signal(SIGINT, onInterrupt);   // before any curses: it keeps a handler already set

    Engine engine;
    engine.init();
    engine.setTargetFrameRate(fps);
    // The games prepare the engine (their services). Which games are there
    // is decided by what is linked into the program, not by this code.
    try {
        GameCatalog::instance().installAll(engine);
    } catch (const std::exception& error) {
        std::fprintf(stderr, "A game failed to install: %s\n", error.what());
        engine.shutdown();
        return 1;
    }

    bool loadedWithErrors = false;
    // The setup adds StopOnInterruptSystem: TEMPORARY, see the comment at the top.
    addDataScene(
        engine.scenes().registry(), "main", file,
        [&engine](DataScene& scene) { scene.systems().addSystem<StopOnInterruptSystem>(scene.registry(), engine); },
        [&loadedWithErrors](const std::string&, const serial::Report& report) { loadedWithErrors = !report.ok(); });

    if (!engine.scenes().push("main")) {
        std::fprintf(stderr, "Cannot run '%s' (see the errors above).\n", file.c_str());
        engine.shutdown();
        return 1;
    }

    const int code = engine.run();
    engine.shutdown();   // destroys the scenes, then the services (a game's terminal or window is released here)
    if (loadedWithErrors)
        std::fprintf(stderr, "'%s' ran with load errors (see above).\n", file.c_str());
    return code;
}
