/*
    Created on: 11/17/26
    Filename: RectShape.hpp
    Description: A filled rectangle whose top-left corner is the Transform.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#pragma once

#include "Components/Layer.hpp"

struct RectShape
{
    Vec2f size{100.0f, 20.0f};
    Rgba color;
    DrawLayer layer = DrawLayer::Game;
    bool topEdge = false;       // a lighter line along the top, as on the platforms
    bool screenSpace = false;   // ignores the camera, e.g. the HUD bar
};

E_REFLECT(RectShape)
    E_FIELD(size, Vec2f)
    E_FIELD(color, Rgba)
    E_FIELD(layer, DrawLayer)
    E_FIELD(topEdge, bool)
    E_FIELD(screenSpace, bool)
E_END
