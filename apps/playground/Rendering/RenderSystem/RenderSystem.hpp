/*
    Created on: 11/17/26
    Filename: RenderSystem.hpp
    Description: Draws every visible entity of the world through IRenderer.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#pragma once

#include "Renderer/RendererTypes.hpp"

#include "ECS/ECS.hpp"

class TextureCache;

namespace gfx {
class Font;
class IRenderer;
}

// Not an ECS::ISystem: the editor runs systems only during Play, the level must also show while editing.
namespace RenderSystem {

// Between beginFrame() and endFrame(), after setCamera(); viewport is the target size in pixels.
// A null font leaves out the HUD text.
void draw(ECS::Registry &world, gfx::IRenderer &renderer, TextureCache &textures, const gfx::Font *font, gfx::Vec2 viewport);

}
