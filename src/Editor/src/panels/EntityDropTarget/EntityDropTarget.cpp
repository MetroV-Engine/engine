/*
    Created on: 09/11/2026
    Filename: EntityDropTarget.cpp
    Description: Creates the entity of a template dropped on a panel

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "EntityDropTarget.hpp"

#include <imgui.h>

#include <string>
#include <utility>

EntityDropTarget::EntityDropTarget(const std::vector<EntityTemplate> &templates, HierarchyModel &hierarchy,
    PropertyModel &properties, NotificationManager &notifications, const PlayController &play, OnCreated onCreated)
    : _templates(templates),
      _hierarchy(hierarchy),
      _properties(properties),
      _notifications(notifications),
      _play(play),
      _onCreated(std::move(onCreated))
{
}

void EntityDropTarget::accept(const std::optional<Vec2> &worldPosition) const
{
    if (!_play.isStopped() || !ImGui::BeginDragDropTarget())
        return;
    // Returned only once, when the mouse button is released over the target.
    if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(EntityTemplatePayloadType)) {
        IM_ASSERT(payload->DataSize == sizeof(EntityTemplatePayload));
        const std::uint32_t index = static_cast<const EntityTemplatePayload *>(payload->Data)->index;
        IM_ASSERT(index < _templates.size());
        create(_templates[index], worldPosition);
    }
    ImGui::EndDragDropTarget();
}

void EntityDropTarget::create(const EntityTemplate &entityTemplate, const std::optional<Vec2> &worldPosition) const
{
    const ObjectId id = _hierarchy.create(HierarchyModel::InvalidId, entityTemplate.objectName);
    if (id == HierarchyModel::InvalidId) {
        _notifications.warning("Cannot create \"" + entityTemplate.label + "\"");
        return;
    }
    for (const std::string &component : entityTemplate.components)
        if (!_properties.addComponent(id, component))
            _notifications.warning("Cannot add \"" + component + "\"", "Component of the template \"" + entityTemplate.label + "\".");
    // Refused for an object without a position (an empty entity): nothing to place.
    if (worldPosition)
        _properties.set(id, PositionComponent, PositionProperty, *worldPosition);
    _onCreated(id);
}
