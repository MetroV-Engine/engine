/*
    Created on: 11/17/26
    Filename: PolygonShape.hpp
    Description: A filled polygon whose points are relative to the Transform.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#pragma once

#include "Components/Layer.hpp"

#include <vector>

struct PolygonShape
{
    std::vector<Vec2f> points;
    Rgba color;
    DrawLayer layer = DrawLayer::Background;
};

E_REFLECT(PolygonShape)
    E_FIELD(points, std::vector<Vec2f>)
    E_FIELD(color, Rgba)
    E_FIELD(layer, DrawLayer)
E_END
