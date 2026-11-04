/*
    Created on: 30/03/2026
    Filename: DemoSceneRenderer.hpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#pragma once

#include "scene/DemoScene/DemoScene.hpp"
#include "scene/Selection.hpp"

#include <imgui.h>

// Draws the demo scene in the Viewport until the engine renders the game:
// each object with a Transform is a box (its BoxCollider size, or a small
// marker), fitted to the view; the selected object is highlighted.
class DemoSceneRenderer
{
public:
    DemoSceneRenderer(const DemoScene &scene, const Selection &selection) : _scene(scene), _selection(selection) {}

    void draw(ImDrawList *drawList, ImVec2 min, ImVec2 max) const;

private:
    const DemoScene &_scene;
    const Selection &_selection;
};
