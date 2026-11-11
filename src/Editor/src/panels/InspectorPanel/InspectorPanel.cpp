/*
    Created on: 30/03/2026
    Filename: InspectorPanel.cpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "InspectorPanel.hpp"
#include "panels/AssetPayload.hpp"
#include "ui/Button/Button.hpp"
#include "ui/ThemeManager/ThemeManager.hpp"

#include <imgui_stdlib.h>

#include <algorithm>
#include <cctype>
#include <cfloat>

namespace {

std::string toLower(std::string text)
{
    std::ranges::transform(text, text.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return text;
}

// Drag widgets clamp only when both bounds are set; min == max means unbounded.
float dragMin(const PropertyInfo &info)
{
    return info.min != info.max ? info.min : 0.0f;
}

float dragMax(const PropertyInfo &info)
{
    return info.min != info.max ? info.max : 0.0f;
}

// AlwaysClamp also clamps a 0..0 range (ImGuiSliderFlags_ClampZeroRange), which
// would freeze unbounded fields at 0: it is only used for bounded properties.
ImGuiSliderFlags clampFlags(const PropertyInfo &info)
{
    return info.min != info.max ? ImGuiSliderFlags_AlwaysClamp : ImGuiSliderFlags_None;
}

const char *formatOr(const PropertyInfo &info, const char *fallback)
{
    return info.format.empty() ? fallback : info.format.c_str();
}

} // namespace

InspectorPanel::InspectorPanel(
    HierarchyModel &hierarchy, PropertyModel &properties, const Selection &selection, NotificationManager &notifications)
    : Panel("Inspector"),
      _hierarchy(hierarchy),
      _properties(properties),
      _selection(selection),
      _notifications(notifications)
{
}

void InspectorPanel::drawContent()
{
    if (!_selection.has() || !_hierarchy.exists(_selection.get())) {
        ImGui::TextDisabled("No object selected.");
        ImGui::PushTextWrapPos(0.0f);
        ImGui::TextDisabled("Select an object in the Hierarchy to see and edit its properties.");
        ImGui::PopTextWrapPos();
        return;
    }

    if (const std::string banner = _banner ? _banner() : std::string(); !banner.empty()) {
        ImGui::PushTextWrapPos(0.0f);
        ImGui::TextColored(ThemeManager::active().colors.warning, "%s", banner.c_str());
        ImGui::PopTextWrapPos();
        ImGui::Separator();
    }

    const ObjectId id = _selection.get();
    drawName(id);
    ImGui::Separator();

    // A copy: the list may change (pending add/remove) while it is drawn.
    const std::vector<ComponentInfo> components = _properties.components(id);
    if (components.empty())
        ImGui::TextDisabled("This object has no components.");
    for (const ComponentInfo &component : components)
        drawComponent(id, component);

    ImGui::Spacing();
    drawAddComponent(id);

    std::vector<std::function<void()>> pending;
    pending.swap(_pending);
    for (const auto &change : pending)
        change();
}

void InspectorPanel::drawName(ObjectId id)
{
    // The buffer follows the selection, and the model while not being edited.
    if (_nameObject != id) {
        _nameObject = id;
        _nameBuffer = _hierarchy.name(id);
    }

    ImGui::SetNextItemWidth(-FLT_MIN);
    ImGui::InputText("##Name", &_nameBuffer);
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        if (_nameBuffer.empty() || !_hierarchy.rename(id, _nameBuffer))
            _nameBuffer = _hierarchy.name(id);
    } else if (!ImGui::IsItemActive()) {
        // Renamed elsewhere (e.g. F2 in the Hierarchy).
        _nameBuffer = _hierarchy.name(id);
    }
    ImGui::TextDisabled("%s", _hierarchy.idLabel(id).c_str());
}

void InspectorPanel::drawComponent(ObjectId id, const ComponentInfo &component)
{
    ImGui::PushID(component.name.c_str());
    const bool open = ImGui::CollapsingHeader(component.name.c_str(), ImGuiTreeNodeFlags_DefaultOpen);

    if (ImGui::BeginPopupContextItem("##ComponentMenu")) {
        if (ImGui::MenuItem("Remove Component", nullptr, false, component.removable)) {
            const std::string name = component.name;
            _pending.emplace_back([this, id, name] {
                if (_properties.removeComponent(id, name))
                    _notifications.info("Component removed", name + " from " + _hierarchy.name(id));
            });
        }
        if (!component.removable)
            ImGui::SetItemTooltip("%s is required on every object.", component.name.c_str());
        ImGui::EndPopup();
    }

    if (open && ImGui::BeginTable("##Properties", 2, ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 0.4f);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 0.6f);
        drawProperties(id, component.name, component.properties, "");
        ImGui::EndTable();
    }
    ImGui::PopID();
}

void InspectorPanel::drawProperties(
    ObjectId id, const std::string &component, const std::vector<PropertyInfo> &properties, const std::string &prefix)
{
    for (const PropertyInfo &info : properties) {
        const std::string path = prefix.empty() ? info.name : prefix + "." + info.name;
        ImGui::PushID(info.name.c_str());
        ImGui::TableNextRow();
        ImGui::TableNextColumn();

        if (info.type == PropertyType::Group) {
            // A nested struct: a tree node spanning the row, its fields below.
            const bool open = ImGui::TreeNodeEx(info.name.c_str(),
                ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAllColumns | ImGuiTreeNodeFlags_LabelSpanAllColumns);
            if (open) {
                drawProperties(id, component, info.fields, path);
                ImGui::TreePop();
            }
            ImGui::PopID();
            continue;
        }

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(info.name.c_str());
        if (!info.tooltip.empty())
            ImGui::SetItemTooltip("%s", info.tooltip.c_str());

        ImGui::TableNextColumn();
        PropertyValue value = _properties.get(id, component, path);
        ImGui::BeginDisabled(info.readOnly);
        ImGui::SetNextItemWidth(-FLT_MIN);
        if (drawValue(info, value))
            _properties.set(id, component, path, value);
        // A text field also takes the path of an asset dragged from the Asset Browser.
        if (info.type == PropertyType::String && !info.readOnly)
            if (const std::optional<std::string> asset = droppedAsset())
                _properties.set(id, component, path, *asset);
        ImGui::EndDisabled();
        if (info.readOnly && !info.tooltip.empty())
            ImGui::SetItemTooltip("%s", info.tooltip.c_str());
        ImGui::PopID();
    }
}

bool InspectorPanel::drawValue(const PropertyInfo &info, PropertyValue &value)
{
    if (std::holds_alternative<std::monostate>(value)) {
        ImGui::TextDisabled("(no value)");
        return false;
    }

    switch (info.type) {
    case PropertyType::Bool:
        return ImGui::Checkbox("##Value", &std::get<bool>(value));
    case PropertyType::Int:
        return ImGui::DragInt("##Value", &std::get<int>(value), std::max(info.speed, 0.05f),
            static_cast<int>(dragMin(info)), static_cast<int>(dragMax(info)), formatOr(info, "%d"),
            clampFlags(info));
    case PropertyType::Float:
        return ImGui::DragFloat("##Value", &std::get<float>(value), info.speed, dragMin(info), dragMax(info),
            formatOr(info, "%.3f"), clampFlags(info));
    case PropertyType::String:
        // Written at every keystroke: the value is read back from the model
        // each frame, so it must never lag behind the field.
        return ImGui::InputText("##Value", &std::get<std::string>(value));
    case PropertyType::Vec2:
        return ImGui::DragFloat2("##Value", &std::get<Vec2>(value).x, info.speed, dragMin(info), dragMax(info),
            formatOr(info, "%.3f"), clampFlags(info));
    case PropertyType::Color:
        return ImGui::ColorEdit4("##Value", &std::get<Color>(value).r, ImGuiColorEditFlags_AlphaBar);
    case PropertyType::Enum: {
        int &index = std::get<int>(value);
        const char *preview =
            index >= 0 && index < static_cast<int>(info.enumOptions.size()) ? info.enumOptions[index].c_str() : "?";
        bool changed = false;
        if (ImGui::BeginCombo("##Value", preview)) {
            for (int i = 0; i < static_cast<int>(info.enumOptions.size()); i++) {
                if (ImGui::Selectable(info.enumOptions[i].c_str(), i == index) && i != index) {
                    index = i;
                    changed = true;
                }
            }
            ImGui::EndCombo();
        }
        return changed;
    }
    case PropertyType::Group:
    default:
        return false;
    }
}

void InspectorPanel::drawAddComponent(ObjectId id)
{
    const float width = std::min(ImGui::GetContentRegionAvail().x, ImGui::GetFontSize() * 14.0f);
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetContentRegionAvail().x - width) * 0.5f);
    if (Button("Add Component", {}, Button::Variant::Primary, ImVec2(width, 0.0f)).render()) {
        _componentFilter.clear();
        ImGui::OpenPopup("##AddComponent");
    }

    if (!ImGui::BeginPopup("##AddComponent"))
        return;
    if (ImGui::IsWindowAppearing())
        ImGui::SetKeyboardFocusHere();
    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 14.0f);
    ImGui::InputTextWithHint("##Filter", "Search components", &_componentFilter);
    ImGui::Separator();

    const std::string filter = toLower(_componentFilter);
    bool any = false;
    for (const std::string &name : _properties.addableComponents(id)) {
        if (!filter.empty() && toLower(name).find(filter) == std::string::npos)
            continue;
        any = true;
        if (ImGui::Selectable(name.c_str())) {
            _pending.emplace_back([this, id, name] {
                if (_properties.addComponent(id, name))
                    _notifications.success("Component added", name + " on " + _hierarchy.name(id));
            });
            ImGui::CloseCurrentPopup();
        }
    }
    if (!any)
        ImGui::TextDisabled("No component to add.");
    ImGui::EndPopup();
}
