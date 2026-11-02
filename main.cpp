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

#include "Editor.hpp"
#include "GuiContext.hpp"
#include "log/Logger.hpp"

#include <exception>

int main()
{
    Logger &logger = Logger::instance();
    logger.captureSdlLogs();
    if (!logger.setLogFile("logs/editor.log"))
        logger.warning("Editor", "Cannot write logs/editor.log; logging to the terminal only");

    try {
        GuiContext gui("MetroV Editor", 1600, 900);
        Editor editor(gui);
        editor.run();
    } catch (const std::exception &e) {
        // Kept in logs/editor.log, so the cause of a crash at startup is not lost.
        logger.error("Fatal", e.what());
        return 1;
    }
    return 0;
}
