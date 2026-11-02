/*
    Created on: 30/03/2026
    Filename: InspectorPanel.hpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#pragma once

#include "Panel.hpp"
#include "scene/HierarchyModel.hpp"
#include "scene/Selection.hpp"

// Details of the selected object. For now it shows what the hierarchy knows
// (name, parent, children); once the engine's ECS is connected it will draw
// each component's fields through the reflection layer.
class InspectorPanel : public Panel
{
public:
    InspectorPanel(const HierarchyModel &model, const Selection &selection)
        : Panel("Inspector"), _model(model), _selection(selection)
    {
    }

protected:
    void drawContent() override;

private:
    const HierarchyModel &_model;
    const Selection &_selection;
};
