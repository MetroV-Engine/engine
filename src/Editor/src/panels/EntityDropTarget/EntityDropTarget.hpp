/*
    Created on: 09/11/2026
    Filename: EntityDropTarget.hpp
    Description: Creates the entity of a template dropped on a panel

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#pragma once

#include "scene/EntityTemplate.hpp"
#include "scene/HierarchyModel.hpp"
#include "scene/PlayController/PlayController.hpp"
#include "scene/PropertyModel.hpp"
#include "ui/NotificationManager/NotificationManager.hpp"

#include <cstdint>
#include <functional>
#include <optional>
#include <type_traits>
#include <vector>

inline constexpr const char *EntityTemplatePayloadType = "ENTITY_TEMPLATE";

// What travels during the drag: ImGui copies these bytes, so it holds an index
// into the editor's templates and never a pointer.
struct EntityTemplatePayload
{
    std::uint32_t index;
};
static_assert(std::is_trivially_copyable_v<EntityTemplatePayload>);

// Shared by the panels that accept entity templates. Nothing is accepted
// unless the game is stopped: what is created during Play disappears on Stop.
class EntityDropTarget
{
public:
    using ObjectId = HierarchyModel::ObjectId;
    using OnCreated = std::function<void(ObjectId)>;

    // Everything given must outlive the target.
    EntityDropTarget(const std::vector<EntityTemplate> &templates, HierarchyModel &hierarchy, PropertyModel &properties,
        NotificationManager &notifications, const PlayController &play, OnCreated onCreated);

    // Makes the last drawn item a drop target. A template dropped on it
    // creates its object, at worldPosition if the panel knows where in the
    // world the mouse is.
    void accept(const std::optional<Vec2> &worldPosition = std::nullopt) const;

private:
    const std::vector<EntityTemplate> &_templates;
    HierarchyModel &_hierarchy;
    PropertyModel &_properties;
    NotificationManager &_notifications;
    const PlayController &_play;
    OnCreated _onCreated;

    void create(const EntityTemplate &entityTemplate, const std::optional<Vec2> &worldPosition) const;
};
