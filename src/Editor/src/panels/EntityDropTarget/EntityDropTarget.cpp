/*
    Created on: 09/11/2026
    Filename: EntityDropTarget.cpp
    Description: Lets a panel receive the entity templates dragged from the Entity Palette

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "EntityDropTarget.hpp"
#include "scene/EntityTemplate.hpp"

#include <imgui.h>

#include <cstring>
#include <utility>

EntityDropTarget::EntityDropTarget(const PlayController &play, Handler onDrop) : _play(play), _onDrop(std::move(onDrop)) {}

void EntityDropTarget::accept(const std::optional<Vec2> &worldPosition) const
{
    if (!_play.isStopped() || !ImGui::BeginDragDropTarget())
        return;
    // Returned only once, when the mouse button is released over the target.
    const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(EntityTemplatePayloadType);
    if (payload && payload->DataSize == sizeof(EntityTemplatePayload)) {
        EntityTemplatePayload dropped;
        std::memcpy(&dropped, payload->Data, sizeof(dropped));
        _onDrop(dropped.index, worldPosition);
    }
    ImGui::EndDragDropTarget();
}
