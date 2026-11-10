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
#include "scene/ViewportView.hpp"

#include <imgui.h>

#include <cstddef>

// Draws the demo scene in the Viewport until the engine renders the game:
// each object with a Transform is a box (its BoxCollider size, or a small
// marker); the selected object is highlighted. The view is fitted to the
// objects, then kept while they stay in it, so an object dragged with the
// mouse stays under it. It is fitted again when the Viewport is resized,
// objects are added or removed, or one leaves the view.
class DemoSceneRenderer
{
public:
    DemoSceneRenderer(const DemoScene &scene, const Selection &selection) : _scene(scene), _selection(selection) {}

    // Returns the view used; invalid when the scene has nothing to show.
    ViewportView draw(ImDrawList *drawList, ImVec2 min, ImVec2 max);

private:
    const DemoScene &_scene;
    const Selection &_selection;
    // The view fitted last, and what it was fitted for.
    ViewportView _view;
    ImVec2 _fittedMin;
    ImVec2 _fittedMax;
    std::size_t _fittedObjects = 0;
};
