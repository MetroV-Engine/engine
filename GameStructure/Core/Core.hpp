/*
    Created on: 2/19/26
    Filename: Core.hpp

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/


#ifndef PROTO_GAME_CORE_HPP
#define PROTO_GAME_CORE_HPP

#include <SDL3/SDL.h>
#include <atomic>
#include <mutex>
#include <vector>

#include "System/ISystem.hpp"

namespace Game {

    class Core {
        public:
            static Core &getInstance();

            void createWindow();
            void destroyWindow();
            void pushEvent(const SDL_Event &event);

            void run(ECS::registry &registry, std::vector<ECS::System::ISystem *> &systems, std::atomic<bool> &stopFlag);
            bool tick(ECS::registry &registry, std::vector<ECS::System::ISystem *> &systems);

            SDL_Window *getWindow() const { return _window; }
            bool isOpen() const { return _open; }

        private:
            Core();
            ~Core();

            SDL_Window *_window = nullptr;
            bool _open = false;
            bool _bgfxInitialized = false;
            Uint64 _lastTick = 0;
            float _deltaTime = 0.0f;
            uint16_t _width = 0;
            uint16_t _height = 0;

            std::mutex _eventMutex;
            std::vector<SDL_Event> _pendingEvents;
    };
}

#endif //PROTO_GAME_CORE_HPP
