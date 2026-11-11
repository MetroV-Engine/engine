/*
    Created on: 09/11/2026
    Filename: EntityPalettePanel.hpp
    Description: Panel listing the entity templates that can be dragged into the scene

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#pragma once

#include "panels/Panel/Panel.hpp"
#include "scene/EntityTemplate.hpp"
#include "scene/PlayController/PlayController.hpp"

#include <vector>

// The entity templates, to drag onto the Hierarchy or the Viewport. Dragging
// is only possible while the game is stopped: what is created during Play
// disappears on Stop.
class EntityPalettePanel : public Panel
{
public:
    // templates and play must outlive the panel.
    EntityPalettePanel(const std::vector<EntityTemplate> &templates, const PlayController &play);

protected:
    void drawContent() override;

private:
    const std::vector<EntityTemplate> &_templates;
    const PlayController &_play;
};
