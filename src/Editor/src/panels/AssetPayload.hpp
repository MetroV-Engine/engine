/*
    Created on: 10/11/2026
    Filename: AssetPayload.hpp
    Description: An asset dragged from the Asset Browser onto a field

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#pragma once

#include <imgui.h>

#include <cstring>
#include <optional>
#include <string>
#include <type_traits>

// An asset dragged from the Asset Browser: its path in the assets folder,
// copied into the payload (ImGui copies payloads, so it holds no pointer).
inline constexpr const char *AssetPayloadType = "ASSET_PATH";

struct AssetPayload
{
    char path[512];
};
static_assert(std::is_trivially_copyable_v<AssetPayload>);

// Call right after the item to drag. A path too long for the payload cannot be dragged.
inline void dragAsset(const std::string &path)
{
    if (path.size() >= sizeof(AssetPayload::path) || !ImGui::BeginDragDropSource())
        return;
    AssetPayload payload{};
    std::memcpy(payload.path, path.c_str(), path.size() + 1);
    ImGui::SetDragDropPayload(AssetPayloadType, &payload, sizeof(payload));
    ImGui::TextUnformatted(path.c_str());
    ImGui::EndDragDropSource();
}

// Call right after a field: the path of an asset dropped on it, if any.
inline std::optional<std::string> droppedAsset()
{
    std::optional<std::string> path;
    if (!ImGui::BeginDragDropTarget())
        return path;
    if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(AssetPayloadType)) {
        IM_ASSERT(payload->DataSize == sizeof(AssetPayload));
        path = static_cast<const AssetPayload *>(payload->Data)->path;
    }
    ImGui::EndDragDropTarget();
    return path;
}
