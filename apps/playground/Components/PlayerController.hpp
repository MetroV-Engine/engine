/*
    Created on: 11/17/26
    Filename: PlayerController.hpp
    Description: The bull's walking and hopping: tuning values and current state.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#pragma once

#include "ECS/Reflection/Reflection.hpp"

// The Transform is the point under the hooves; y grows downwards.
struct PlayerController
{
    float walkSpeed = 300.0f;       // world units per second
    float jumpSpeed = 800.0f;       // about one bull high (v^2 / 2g = 120) and 0.6 s in the air
    float gravity = 2667.0f;
    float groundY = 600.0f;
    float landingSeconds = 0.1f;    // how long the landing pose shows

    float velocityY = 0.0f;
    float landingLeft = 0.0f;
    bool airborne = false;
    bool walking = false;
    bool facingLeft = false;        // the sheets are drawn facing right
};

E_REFLECT(PlayerController)
    E_FIELD(walkSpeed, float)
    E_FIELD(jumpSpeed, float)
    E_FIELD(gravity, float)
    E_FIELD(groundY, float)
    E_FIELD(landingSeconds, float)
    E_FIELD(velocityY, float)
    E_FIELD(landingLeft, float)
    E_FIELD(airborne, bool)
    E_FIELD(walking, bool)
    E_FIELD(facingLeft, bool)
E_END
