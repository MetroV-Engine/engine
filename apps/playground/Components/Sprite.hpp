/*
    Created on: 11/17/26
    Filename: Sprite.hpp
    Description: One frame of a sprite sheet, drawn at the Transform.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#pragma once

#include "Components/Layer.hpp"

#include <string>

// Frames are laid out left to right on the first row of the sheet.
struct Sprite
{
    std::string sheet;                          // asset name, e.g. "bongo_marche_spritesheet.png"
    int frame = 0;
    Vec2f frameSize{512.0f, 512.0f};
    Vec2f drawSize{128.0f, 128.0f};
    Vec2f anchor{0.5f, 460.0f / 512.0f};        // the point of the frame put on the Transform: the hooves
    bool flipX = false;
    Rgba tint;
    DrawLayer layer = DrawLayer::Game;
};

E_REFLECT(Sprite)
    E_FIELD(sheet, std::string)
    E_FIELD(frame, int)
    E_FIELD(frameSize, Vec2f)
    E_FIELD(drawSize, Vec2f)
    E_FIELD(anchor, Vec2f)
    E_FIELD(flipX, bool)
    E_FIELD(tint, Rgba)
    E_FIELD(layer, DrawLayer)
E_END
