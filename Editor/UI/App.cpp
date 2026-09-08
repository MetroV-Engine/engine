#include "App.hpp"

#include <iostream>
#include <filesystem>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

#include "Entity/Base/Position/Position.hpp"
#include "Entity/Base/Velocity/Velocity.hpp"
#include "Entity/GameEntities.hpp"
#include "System/Pong/DrawableSys/DrawableSys.hpp"
#include "System/Pong/BallSys/BallSys.hpp"
#include "System/Pong/ControlsSys/ControlsSys.hpp"
#include "System/Pong/MovementSys/MovementSys.hpp"
#include "System/Base/CollisionSys/CollisionSys.hpp"

#include "Save/SaveManager/SaveManager.hpp"
#include "Scripting/ScriptEngine.hpp"
#include "ProjectContext.hpp"
#include "GameStructure/Core/Core.hpp"

#include "providers/Theme/ThemeProvider.hpp"
#include "providers/Scale/ScaleProvider.hpp"
#include "providers/Selection/SelectionProvider.hpp"

#include "components/ToolBar/ToolBar.hpp"
#include "components/SceneHierarchy/SceneHierarchy.hpp"
#include "components/Viewport/Viewport.hpp"
#include "components/Inspector/Inspector.hpp"

namespace Editor::UI {

    App &App::getInstance()
    {
        static App instance;
        return instance;
    }

    App::App()
    {
        initSDL();
        initImGui();

        Editor::SaveManager::getInstance().load(_reg);

        registerSystems();
        buildEditorPage();

        // Boot Mono + load scripts assembly
        auto &scriptEngine = Scripting::ScriptEngine::getInstance();
        scriptEngine.init();

        const auto scriptsDir = Editor::ProjectContext::getInstance().resolve("scripts");
        const auto dllPath    = scriptsDir / "Scripts.dll";
        if (std::filesystem::exists(scriptsDir)) {
            Scripting::ScriptEngine::recompileScripts(scriptsDir.string(), dllPath.string());
            if (std::filesystem::exists(dllPath))
                scriptEngine.loadAssembly(dllPath.string());
        }
    }

    App::~App()
    {
        shutdownImGui();
        shutdownSDL();
    }

    void App::initSDL()
    {
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
            std::exit(1);
        }

        Providers::useScale().initWinSize();
        const auto winSize = Providers::useScale().getWinSize();

        _window = SDL_CreateWindow("Proto", winSize.x, winSize.y, SDL_WINDOW_RESIZABLE);
        if (!_window) {
            std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << std::endl;
            SDL_Quit();
            std::exit(1);
        }

        _renderer = SDL_CreateRenderer(_window, nullptr);
        if (!_renderer) {
            std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << std::endl;
            SDL_DestroyWindow(_window);
            SDL_Quit();
            std::exit(1);
        }
    }

    void App::initImGui()
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiIO &io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        const float scaleY = Providers::useScale().getScale().y;
        io.Fonts->AddFontFromFileTTF(
            PROJECT_ROOT_DIR "/External/imgui/misc/fonts/Roboto-Medium.ttf",
            18.0f * scaleY);

        Providers::ThemeProvider::apply(scaleY);

        ImGui_ImplSDL3_InitForSDLRenderer(_window, _renderer);
        ImGui_ImplSDLRenderer3_Init(_renderer);
    }

    void App::registerSystems()
    {
        _systems.push_back(new ECS::System::DrawableSys);
        _systems.push_back(new ECS::System::ControlsSys);
        _systems.push_back(new ECS::System::MovementSys);
        _systems.push_back(new ECS::System::BallSys);
        _systems.push_back(new ECS::System::CollisionSys);

        for (auto *sys : _systems)
            _editorPage.setSystemEnabled(sys, true);
    }

    void App::buildEditorPage()
    {
        _editorPage.addPanel(std::make_shared<Components::SceneHierarchy>(&_editorPage));
        _editorPage.addPanel(std::make_shared<Components::Viewport>());
        _editorPage.addPanel(std::make_shared<Components::Inspector>());
        auto toolbar = std::make_shared<Components::ToolBar>(&_editorPage);
        _toolbar = toolbar.get();
        _editorPage.addPanel(toolbar);
    }

    void App::processEvents()
    {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            // Skip events targeting the game window — Core::tick() handles them
            auto &gameCore = Game::Core::getInstance();
            SDL_Window *gameWin = gameCore.getWindow();
            if (gameWin && gameCore.isOpen()) {
                SDL_WindowID gameWinID = SDL_GetWindowID(gameWin);
                bool isGameEvent = false;
                if (event.type >= SDL_EVENT_WINDOW_FIRST && event.type <= SDL_EVENT_WINDOW_LAST)
                    isGameEvent = (event.window.windowID == gameWinID);
                if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP)
                    isGameEvent = (event.key.windowID == gameWinID);
                if (isGameEvent) {
                    SDL_PushEvent(&event);  // Put it back for Core::tick() to consume
                    continue;
                }
            }

            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT) {
                _running = false;
            }
            if (event.type == SDL_EVENT_WINDOW_RESIZED || event.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED) {
                int w, h;
                SDL_GetWindowSize(_window, &w, &h);
                Providers::useScale().setWinSize(Proto::Vec2u(w, h));
                Providers::useScale().setScale(Proto::Vec2f(
                    static_cast<float>(w) / BASE_RESOLUTION_W,
                    static_cast<float>(h) / BASE_RESOLUTION_H));
            }
        }
    }

    void App::run()
    {
        while (_running) {
            processEvents();

            ImGui_ImplSDLRenderer3_NewFrame();
            ImGui_ImplSDL3_NewFrame();
            ImGui::NewFrame();

            Scripting::ScriptEngine::getInstance().checkHotReload(_reg);

            _editorPage.render(_reg, _systems);

            if (ImGui::IsKeyPressed(ImGuiKey_Escape) && !ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId)) {
                Providers::useSelection().clearSelection();
            }

            ImGui::Render();
            SDL_SetRenderDrawColor(_renderer, 0, 0, 0, 255);
            SDL_RenderClear(_renderer);
            ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), _renderer);
            SDL_RenderPresent(_renderer);

            // Tick the game (one frame) if running — runs on main thread
            if (_toolbar) {
                _toolbar->playButton().tickGame(_reg, _editorPage.enabledSystems());
            }
        }

        Scripting::ScriptEngine::getInstance().shutdown();
        Editor::SaveManager::getInstance().save(_reg);

        for (auto *sys : _systems) delete sys;
    }

    void App::shutdownImGui()
    {
        ImGui_ImplSDLRenderer3_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
    }

    void App::shutdownSDL()
    {
        if (_renderer) SDL_DestroyRenderer(_renderer);
        if (_window)   SDL_DestroyWindow(_window);
        SDL_Quit();
    }

}
