/*
    Created on: 11/17/26
    Filename: CircleShape.hpp
    Description: A filled circle centred on the Transform.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#pragma once

#include "Components/Layer.hpp"

struct CircleShape
{
    float radius = 10.0f;
    Rgba color;
    DrawLayer layer = DrawLayer::Game;
    bool screenSpace = false;
    bool viewportAnchor = false;    // screen space only: the Transform is a fraction of the viewport (the sun)
};

E_REFLECT(CircleShape)
    E_FIELD(radius, float)
    E_FIELD(color, Rgba)
    E_FIELD(layer, DrawLayer)
    E_FIELD(screenSpace, bool)
    E_FIELD(viewportAnchor, bool)
E_END
