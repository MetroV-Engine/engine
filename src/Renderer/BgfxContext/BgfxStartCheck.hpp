/*
    Created on: 17/11/2026
    Filename: BgfxStartCheck.hpp
    Description: Whether bgfx started the renderer it was asked for.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#ifndef RENDERER_WRAPPER_BGFXSTARTCHECK_HPP
#define RENDERER_WRAPPER_BGFXSTARTCHECK_HPP

#include <bgfx/bgfx.h>

namespace gfx {

    // Why the renderer bgfx started is unusable, or nullptr when it is fine.
    // bgfx falls back to Noop when every GPU renderer fails: a window that never draws, so it is a failure here.
    inline const char* bgfxStartFailure(const bool noopAsked, const bgfx::RendererType::Enum started)
    {
        if (!noopAsked && started == bgfx::RendererType::Noop) {
            return "no GPU renderer could start (bgfx fell back to Noop)";
        }
        return nullptr;
    }

}

#endif //RENDERER_WRAPPER_BGFXSTARTCHECK_HPP
