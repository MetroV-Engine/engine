/*
    Created on: 09/11/2026
    Filename: EntityTemplate.hpp
    Description: Entity templates offered by the Entity Palette

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#pragma once

#include <string>
#include <vector>

// A kind of entity that can be dragged from the Entity Palette into the scene.
struct EntityTemplate
{
    // Shown in the palette.
    std::string label;
    // Name given to the created object.
    std::string objectName;
    // Components added to it, by reflected type name.
    std::vector<std::string> components;
};
