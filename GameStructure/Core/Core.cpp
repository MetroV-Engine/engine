/*
    Created on: 2/19/26
    Filename: Core.cpp

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/


#include "Core.hpp"
#include "PlatformUtils.hpp"
#include "Scripting/ScriptEngine.hpp"
#include "System/ISystem.hpp"
#include "ProjectContext.hpp"

#include <bgfx/bgfx.h>
#include <bgfx/platform.h>

namespace Game {
    Core::Core()
        : _deltaTime(0.0f)
    {
    }

    Core::~Core()
    {
        if (_window) SDL_DestroyWindow(_window);
        _window = nullptr;
    }

    Core & Core::getInstance()
    {
        static Core instance;
        return instance;
    }

    void Core::createWindow()
    {
        if (_window) {
            SDL_DestroyWindow(_window);
            _window = nullptr;
        }

        auto& ctx = Editor::ProjectContext::getInstance();
        _width = static_cast<uint16_t>(ctx.screenWidth());
        _height = static_cast<uint16_t>(ctx.screenHeight());

        _window = SDL_CreateWindow("Proto", _width, _height, SDL_WINDOW_RESIZABLE);
        _open = (_window != nullptr);
        if (!_open) return;

        float aspect = static_cast<float>(_width) / static_cast<float>(_height);
        SDL_SetWindowAspectRatio(_window, aspect, aspect);

        bgfx::PlatformData pd{};
        pd.nwh = getNativeWindowHandle(_window);
        pd.ndt = getNativeDisplayHandle(_window);

        if (!_bgfxInitialized) {
            bgfx::renderFrame();

            bgfx::Init init;
            init.type = bgfx::RendererType::Count;
            init.resolution.width = _width;
            init.resolution.height = _height;
            init.resolution.reset = BGFX_RESET_VSYNC;
            init.platformData = pd;

            if (!bgfx::init(init)) {
                SDL_DestroyWindow(_window);
                _window = nullptr;
                _open = false;
                return;
            }
            _bgfxInitialized = true;
        } else {
            bgfx::setPlatformData(pd);
            bgfx::reset(_width, _height, BGFX_RESET_VSYNC);
        }

        bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x000000ff, 1.0f, 0);
        bgfx::setViewRect(0, 0, 0, _width, _height);

        bgfx::touch(0);
        bgfx::frame();

        _lastTick = SDL_GetPerformanceCounter();
        _deltaTime = 0.0f;

        std::lock_guard<std::mutex> lock(_eventMutex);
        _pendingEvents.clear();
    }

    void Core::destroyWindow()
    {
        _open = false;

        if (_bgfxInitialized) {
            bgfx::frame();
        }
        if (_window) {
            SDL_DestroyWindow(_window);
            _window = nullptr;
        }
    }

    void Core::pushEvent(const SDL_Event &event)
    {
        std::lock_guard<std::mutex> lock(_eventMutex);
        _pendingEvents.push_back(event);
    }

    bool Core::tick(ECS::registry &registry, std::vector<ECS::System::ISystem *> &systems)
    {
        if (!_open) return false;

        Uint64 now = SDL_GetPerformanceCounter();
        _deltaTime = static_cast<float>(now - _lastTick) / static_cast<float>(SDL_GetPerformanceFrequency());
        _lastTick = now;

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            // Only handle events targeting the game window
            SDL_WindowID gameWinId = _window ? SDL_GetWindowID(_window) : 0;
            bool isGameEvent = false;

            if (event.type >= SDL_EVENT_WINDOW_FIRST && event.type <= SDL_EVENT_WINDOW_LAST) {
                isGameEvent = (event.window.windowID == gameWinId);
            } else if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP) {
                isGameEvent = (event.key.windowID == gameWinId);
            }

            if (!isGameEvent) {
                // Push non-game events back for the editor to handle next frame
                SDL_PushEvent(&event);
                continue;
            }

            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
                _open = false;
            }
            if (event.type == SDL_EVENT_KEY_DOWN && event.key.scancode == SDL_SCANCODE_ESCAPE) {
                _open = false;
            }
            if (event.type == SDL_EVENT_WINDOW_RESIZED) {
                _width = static_cast<uint16_t>(event.window.data1);
                _height = static_cast<uint16_t>(event.window.data2);
                bgfx::reset(_width, _height, BGFX_RESET_VSYNC);
                bgfx::setViewRect(0, 0, 0, _width, _height);
            }
        }

        if (!_open) return false;

        bgfx::touch(0);

        Editor::Scripting::ScriptEngine::getInstance().tickAll(registry, _deltaTime);

        for (const auto &sys : systems) {
            sys->update(registry, _deltaTime, _window);
        }

        bgfx::frame();
        return true;
    }

    void Core::run(ECS::registry &registry, std::vector<ECS::System::ISystem *> &systems, std::atomic<bool> &stopFlag)
    {
        while (_open && !stopFlag.load()) {
            if (!tick(registry, systems))
                break;
        }
        _open = false;
    }
}
