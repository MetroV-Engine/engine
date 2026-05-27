/*
    Created on: 3/2/26
    Filename: DrawableSys.hpp

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/


#ifndef PROTO_DRAWABLESYS_HPP
#define PROTO_DRAWABLESYS_HPP

#include "System/ISystem.hpp"
#include <bgfx/bgfx.h>

namespace ECS::System {
    class DrawableSys : public ISystem {
        public:
            DrawableSys();
            ~DrawableSys() override;
            void update(registry &r, float deltaTime, SDL_Window *win) override;
            [[nodiscard]] const char* getName() const override {return "DrawableSys";};
        private:
            bgfx::ProgramHandle _program = BGFX_INVALID_HANDLE;
            bgfx::VertexLayout _layout;
    };
}

#endif //PROTO_DRAWABLESYS_HPP
