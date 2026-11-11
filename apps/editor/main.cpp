/*
    Created on: 30/03/2026
    Filename: main.cpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "core/Editor/Editor.hpp"
#include "core/GuiContext/GuiContext.hpp"
#include "core/Logger/Logger.hpp"

#include "ECS/ECS.hpp"

#include <cstdio>
#include <exception>
#include <string>

namespace {

void printUsage(const char *program)
{
    std::printf("Usage: %s [--demo]\n\n"
                "Without option, edits an empty ECS world.\n\n"
                "  --demo, -d   Edit the built-in demo level instead, with the widget demo panels\n"
                "  --help, -h   Show this help\n",
        program);
}

} // namespace

int main(int argc, char **argv)
{
    Editor::Options options;
    for (int i = 1; i < argc; i++) {
        const std::string arg = argv[i];
        if (arg == "--demo" || arg == "-d") {
            options.demo = true;
        } else if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 0;
        } else {
            std::fprintf(stderr, "Unknown option: %s\n\n", arg.c_str());
            printUsage(argv[0]);
            return 2;
        }
    }

    Logger &logger = Logger::instance();
    logger.captureSdlLogs();
    if (!logger.setLogFile("logs/editor.log"))
        logger.warning("Editor", "Cannot write logs/editor.log; logging to the terminal only");

    try {
        GuiContext gui(options.demo ? "MetroV Editor (demo)" : "MetroV Editor", 1600, 900);
        if (options.demo) {
            Editor editor(gui, options);
            editor.run();
        } else {
            // The world outlives the editor. A game passes its own world and
            // systems instead (see examples/EcsExample.cpp).
            ECS::Registry world;
            ECS::SystemManager systems;
            Editor editor(gui, world, systems, options);
            editor.run();
        }
    } catch (const std::exception &e) {
        // Kept in logs/editor.log, so the cause of a crash at startup is not lost.
        logger.error("Fatal", e.what());
        return 1;
    }
    return 0;
}
