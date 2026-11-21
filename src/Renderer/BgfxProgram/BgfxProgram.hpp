/*
    Created on: 10/4/26
    Filename: BgfxProgram.hpp
    Description: RAII sprite shader program, embedded for the API bgfx picked.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#ifndef RENDERER_WRAPPER_BGFXPROGRAM_HPP
#define RENDERER_WRAPPER_BGFXPROGRAM_HPP

#include <bgfx/bgfx.h>

namespace gfx {

    // Construct after bgfx::init() and destroy before bgfx::shutdown(); check valid().
    class BgfxProgram {
        public:
            BgfxProgram();
            ~BgfxProgram();

            BgfxProgram(const BgfxProgram&) = delete;
            BgfxProgram& operator=(const BgfxProgram&) = delete;

            bool valid() const;
            bgfx::ProgramHandle handle() const;

        private:
            bgfx::ProgramHandle m_program = BGFX_INVALID_HANDLE;
    };

}

#endif //RENDERER_WRAPPER_BGFXPROGRAM_HPP
