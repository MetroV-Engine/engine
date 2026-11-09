/*
    Created on: 09/11/2026
    Filename: EntityPalettePanel.cpp
    Description: Panel listing the entity templates that can be dragged into the scene

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "EntityPalettePanel.hpp"

#include <cstdint>

EntityPalettePanel::EntityPalettePanel(const std::vector<EntityTemplate> &templates, const PlayController &play)
    : Panel("Entity Palette"), _templates(templates), _play(play)
{
}

void EntityPalettePanel::drawContent()
{
    const bool stopped = _play.isStopped();
    if (!stopped)
        ImGui::TextDisabled("Stop the game to add entities.");

    ImGui::BeginDisabled(!stopped);
    for (std::uint32_t i = 0; i < _templates.size(); i++) {
        const EntityTemplate &entityTemplate = _templates[i];
        ImGui::PushID(static_cast<int>(i));
        ImGui::Selectable(entityTemplate.label.c_str());
        ImGui::SetItemTooltip("Drag onto the Hierarchy or the Viewport");

        // A source that stops being submitted (Play pressed during the drag)
        // cancels the drag: without PayloadAutoExpire it would stay active
        // until the mouse button is released.
        if (stopped && ImGui::BeginDragDropSource(ImGuiDragDropFlags_PayloadAutoExpire)) {
            const EntityTemplatePayload payload{i};
            ImGui::SetDragDropPayload(EntityTemplatePayloadType, &payload, sizeof(payload));
            ImGui::TextUnformatted(entityTemplate.label.c_str());
            ImGui::EndDragDropSource();
        }
        ImGui::PopID();
    }
    ImGui::EndDisabled();
}
