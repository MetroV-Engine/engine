/*
    Created on: 2/20/26
    Filename: ISystem.hpp

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/


#ifndef PROTO_ISYSTEM_HPP
#define PROTO_ISYSTEM_HPP

#include "Registry.hpp"
#include "Zipper.hpp"

#include <SDL3/SDL.h>

namespace ECS::System {
    class ISystem {

    public:
        virtual ~ISystem() = default;
        virtual void update(registry& r, float dt, SDL_Window *win) = 0;
        [[nodiscard]] virtual const char* getName() const = 0;
    };
}
#endif //PROTO_ISYSTEM_HPP
