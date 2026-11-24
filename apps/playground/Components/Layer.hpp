/*
    Created on: 11/17/26
    Filename: Layer.hpp
    Description: Value types shared by the components: a 2D vector, a colour, a draw layer.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#pragma once

#include "ECS/Reflection/Reflection.hpp"

// {x, y}: the Inspector shows it as a Vec2.
struct Vec2f
{
    float x = 0.0f;
    float y = 0.0f;
};

// Floats 0..1; {r, g, b, a}: the Inspector shows it as a Color.
struct Rgba
{
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
    float a = 1.0f;
};

// Background is drawn first, Foreground last.
enum class DrawLayer
{
    Background,
    Game,
    Foreground,
};

E_REFLECT(Vec2f)
    E_FIELD(x, float)
    E_FIELD(y, float)
E_END

E_REFLECT(Rgba)
    E_FIELD(r, float)
    E_FIELD(g, float)
    E_FIELD(b, float)
    E_FIELD(a, float)
E_END
