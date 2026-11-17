/*
    Created on: 11/17/26
    Filename: Camera.hpp
    Description: The game camera: its Transform is the world point at the centre of the view.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#pragma once

#include "ECS/Reflection/Reflection.hpp"

#include <string>

struct Camera
{
    float zoom = 1.0f;          // > 1 magnifies
    std::string followTarget;   // name of the entity whose x it follows during play; empty: it stays put
};

E_REFLECT(Camera)
    E_FIELD(zoom, float)
    E_FIELD(followTarget, std::string)
E_END
