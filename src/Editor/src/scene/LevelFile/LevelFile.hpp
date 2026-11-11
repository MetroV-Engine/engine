/*
    Created on: 10/11/2026
    Filename: LevelFile.hpp
    Description: Writes a scene as a level file, and reads it back

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#pragma once

#include "scene/HierarchyModel.hpp"
#include "scene/PropertyModel.hpp"

#include <string>
#include <vector>

// A scene as text, to save levels and open them again. It only goes through
// the HierarchyModel and the PropertyModel, so it works for any scene (an ECS
// world, the demo): every object with its name, its parent and its
// components, with the values of their editable properties. Read-only
// properties (lists, maps...) and what the models do not show (resources,
// unreflected components) are not saved.
//
//   metrov-level 1
//   object 0 - "Player"
//   component "Transform"
//   property "position" vec2 64 160
//   property "movement.speed" float 2.5
//   object 1 0 "Sword"
//
// Objects come in display order, each one after its parent: "object <index>
// <parent index, or - for a root> <name>". Strings are quoted, with \", \\
// and \n escapes.
class LevelFile
{
public:
    static constexpr int Version = 1;

    static std::string write(const HierarchyModel &hierarchy, const PropertyModel &properties);
    // A level without objects.
    static std::string empty();
    // Replaces every object of the scene by the level's. Throws
    // std::runtime_error, leaving the scene untouched, if text is not a valid
    // level; returns what the scene refused (an unknown component...).
    static std::vector<std::string> read(const std::string &text, HierarchyModel &hierarchy, PropertyModel &properties);
};
