/*
    Created on: 17/11/2026
    Filename: BgfxDrawState.hpp
    Description: The bgfx render state every sprite, shape and text draw uses.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#ifndef RENDERER_WRAPPER_BGFXDRAWSTATE_HPP
#define RENDERER_WRAPPER_BGFXDRAWSTATE_HPP

#include <bgfx/defines.h>

#include <cstdint>

namespace gfx::bgfxDrawState {

    // No depth: order is submission order inside one sequential view. Alpha is blended like SDL's BLEND mode
    // (ONE, INV_SRC_ALPHA), so an offscreen target stays opaque where it was cleared opaque.
    constexpr uint64_t Sprite = BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A
        | BGFX_STATE_BLEND_FUNC_SEPARATE(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA,
                                         BGFX_STATE_BLEND_ONE, BGFX_STATE_BLEND_INV_SRC_ALPHA);

}

#endif //RENDERER_WRAPPER_BGFXDRAWSTATE_HPP
