/*
    Created on: 11/17/26
    Filename: BgfxTextureId.hpp
    Description: How a bgfx texture travels as an opaque id (outputTexture(), ImTextureID).

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#ifndef RENDERER_WRAPPER_BGFXTEXTUREID_HPP
#define RENDERER_WRAPPER_BGFXTEXTUREID_HPP

#include <cstdint>

namespace gfx::bgfxTextureId {

    // Index + 1, so 0 stays invalid like ImTextureID_Invalid while bgfx index 0 remains usable.
    constexpr std::uintptr_t encode(const uint16_t index)
    {
        return static_cast<std::uintptr_t>(index) + 1u;
    }

    constexpr uint16_t decode(const std::uintptr_t id)
    {
        return static_cast<uint16_t>(id - 1u);
    }

}

#endif //RENDERER_WRAPPER_BGFXTEXTUREID_HPP
