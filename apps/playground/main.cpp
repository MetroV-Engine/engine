/*
    Created on: 11/17/26
    Filename: main.cpp
    Description: The playground game in the editor: an ECS world drawn through IRenderer into the Viewport.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

// The game: sky, hills, platforms and a bull (A/D or arrows walk, Space hops),
// built from ECS entities that the Hierarchy lists and the Inspector edits.
// Play (F5) runs the systems, Stop puts the level back.
//
//   ./build/all/playground                    SDL3
//   ./build/bgfx/playground --backend=bgfx   bgfx (the bgfx preset)

#include "Level/Level/Level.hpp"
#include "Rendering/TextureCache/TextureCache.hpp"
#include "Systems/AnimationSystem/AnimationSystem.hpp"
#include "Systems/CameraFollowSystem/CameraFollowSystem.hpp"
#include "Systems/PlayerSystem/PlayerSystem.hpp"
#include "ViewportView/ViewportView.hpp"

#include "Renderer/BasePath/BasePath.hpp"
#include "Renderer/Font/Font.hpp"
#include "Renderer/RendererFactory/RendererFactory.hpp"

#include "core/Editor/Editor.hpp"
#include "core/GuiContext/GuiContext.hpp"
#include "core/GuiContext/UiBackendOption.hpp"
#include "core/Logger/Logger.hpp"

#include "ECS/ECS.hpp"

#include <cstdint>
#include <cstdio>
#include <exception>
#include <fstream>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace {

constexpr float FontPixelHeight = 24.0f;

void printUsage(const char *program)
{
    std::printf("Usage: %s [--backend=sdl|bgfx]\n\n"
                "Opens the editor on the playground game.\n\n"
                "%s"
                "  --help, -h       Show this help\n",
        program, UiBackendOptionHelp);
}

std::vector<uint8_t> readFile(const std::string &path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file)
        return {};
    return std::vector<uint8_t>(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

} // namespace

int main(int argc, char **argv)
{
    UiBackend uiBackend = UiBackend::Sdl;
    for (int i = 1; i < argc; i++) {
        const std::string arg = argv[i];
        if (const auto chosen = parseUiBackendOption(arg)) {
            uiBackend = *chosen;
        } else if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 0;
        } else {
            std::fprintf(stderr, "Unknown option: %s\n\n", arg.c_str());
            printUsage(argv[0]);
            return 2;
        }
    }
    // The game draws with what draws the UI.
    const gfx::Backend backend = uiBackend == UiBackend::Bgfx ? gfx::Backend::Bgfx : gfx::Backend::Sdl3Cpu;
    if (!isUiBackendBuilt(uiBackend)) {
        std::fprintf(stderr, "The bgfx backend is not built (ENGINE_RENDERER_BGFX=OFF): use --backend=sdl\n");
        return 1;
    }
    const bool bgfx = backend == gfx::Backend::Bgfx;

    Logger &logger = Logger::instance();
    logger.captureSdlLogs();
    if (!logger.setLogFile("logs/playground.log"))
        logger.warning("Playground", "Cannot write logs/playground.log; logging to the terminal only");

    try {
        // Declaration order is the lifetime: everything holding a texture dies before the renderer, and it before the window.
        GuiContext gui("MetroV Playground", 1600, 900, uiBackend);
        // The game draws with what draws the UI: into a texture of the editor's SDL_Renderer, or on its bgfx context.
        const std::unique_ptr<gfx::IOffscreenRenderer> renderer = bgfx
            ? gfx::makeOffscreenRenderer(*gui.getBgfxContext(), 1280, 720)
            : gfx::makeOffscreenRenderer(backend, gui.getSdlRenderer(), 1280, 720);
        if (renderer == nullptr) {
            logger.error("Playground", "Cannot create the offscreen renderer");
            return 1;
        }

        const std::vector<uint8_t> ttf = readFile(tools::pathNextToExecutable("assets/DejaVuSans.ttf"));
        const std::unique_ptr<gfx::Font> font = gfx::Font::create(*renderer, ttf, FontPixelHeight);
        if (font == nullptr)
            logger.warning("Playground", "No font: the HUD has no text");
        TextureCache textures(*renderer, tools::pathNextToExecutable("assets/"));

        ECS::Registry world;
        spawnLevel(world);
        ECS::SystemManager systems;
        systems.addSystem<PlayerSystem>(world);
        systems.addSystem<AnimationSystem>(world);
        systems.addSystem<CameraFollowSystem>(world);

        Editor editor(gui, world, systems);
        ViewportView view(world, *renderer, textures, font.get(), [&editor] { return editor.playState(); });
        editor.setViewportRenderer([&view](ImDrawList *drawList, ImVec2 min, ImVec2 max) { view(drawList, min, max); });
        logger.info("Playground", std::string("Rendering through ") + renderer->backendName());
        editor.run();
    } catch (const std::exception &e) {
        logger.error("Fatal", e.what());
        return 1;
    }
    return 0;
}
