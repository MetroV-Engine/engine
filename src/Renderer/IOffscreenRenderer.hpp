/*
    Created on: 11/17/26
    Filename: IOffscreenRenderer.hpp
    Description: A renderer that draws into a texture a UI shows, instead of presenting to a window.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#ifndef RENDERER_WRAPPER_IOFFSCREENRENDERER_HPP
#define RENDERER_WRAPPER_IOFFSCREENRENDERER_HPP

#include "Renderer/IRenderer.hpp"

#include <cstdint>

namespace gfx {

    class IOffscreenRenderer : public IRenderer {
        public:
            // Native texture of the last frame (SDL_Texture* for SDL), as an ImTextureID expects; 0 when there is none.
            virtual std::uintptr_t outputTexture() const = 0;
    };

}

#endif //RENDERER_WRAPPER_IOFFSCREENRENDERER_HPP
