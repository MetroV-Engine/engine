/*
    Created on: 09/11/2026
    Filename: EntityTemplate.hpp
    Description: Entity templates offered by the Entity Palette, and their drag and drop payload

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#pragma once

#include <cstdint>
#include <string>
#include <type_traits>
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

inline constexpr const char *EntityTemplatePayloadType = "ENTITY_TEMPLATE";

// What travels during the drag: ImGui copies these bytes, so it holds an index
// into the editor's templates (checked on drop) and never a pointer.
struct EntityTemplatePayload
{
    std::uint32_t index;
};
static_assert(std::is_trivially_copyable_v<EntityTemplatePayload>);
