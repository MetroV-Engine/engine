/*
    Filename: App.hpp
    Description: Top-level editor application. Owns SDL/ImGui lifecycle,
                 the ECS registry, the systems, and the editor page.
                 Replaces the old Editor::Core monolith.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚════╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#ifndef PROTO_UI_APP_HPP
#define PROTO_UI_APP_HPP

#include <vector>
#include <SDL3/SDL.h>

#include "Registry.hpp"
#include "System/ISystem.hpp"

#include "pages/Editor/EditorPage.hpp"
#include "components/ToolBar/ToolBar.hpp"

namespace Editor::UI {

    class App {
    public:
        static App &getInstance();

        void run();

    private:
        App();
        ~App();

        App(const App &) = delete;
        App &operator=(const App &) = delete;

        void initSDL();
        void initImGui();
        void shutdownImGui();
        void shutdownSDL();
        void registerSystems();
        void buildEditorPage();
        void processEvents();

        SDL_Window   *_window   = nullptr;
        SDL_Renderer *_renderer = nullptr;
        bool _running = true;

        ECS::registry _reg;
        std::vector<ECS::System::ISystem *> _systems;
        Pages::EditorPage _editorPage;
        Components::ToolBar *_toolbar = nullptr;
    };

}

#endif
