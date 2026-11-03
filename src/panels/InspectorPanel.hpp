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
#include "feedback/NotificationManager.hpp"
#include "scene/HierarchyModel.hpp"
#include "scene/PropertyModel.hpp"
#include "scene/Selection.hpp"

#include <functional>
#include <string>
#include <vector>

// Shows and edits the selected object: its name, then each component in a
// collapsible section with one widget per property (checkbox, number, text,
// vector, color, dropdown, nested group). Components can be added and removed.
//
// Values are read from the PropertyModel every frame and written back as soon
// as a widget changes them: the Inspector keeps no copy of the data, so it is
// always in sync with the scene. Adding/removing components is applied after
// the panel is drawn.
class InspectorPanel : public Panel
{
public:
    using ObjectId = HierarchyModel::ObjectId;

    InspectorPanel(HierarchyModel &hierarchy, PropertyModel &properties, const Selection &selection,
        NotificationManager &notifications);

    // Text shown above the properties when it returns a non-empty string
    // (e.g. a warning that play mode changes are not kept).
    void setBanner(std::function<std::string()> banner) { _banner = std::move(banner); }

protected:
    void drawContent() override;

private:
    HierarchyModel &_hierarchy;
    PropertyModel &_properties;
    const Selection &_selection;
    NotificationManager &_notifications;

    // Name being edited, and the object it belongs to.
    std::string _nameBuffer;
    ObjectId _nameObject = HierarchyModel::InvalidId;
    std::string _componentFilter;
    std::vector<std::function<void()>> _pending;
    std::function<std::string()> _banner;

    void drawName(ObjectId id);
    void drawComponent(ObjectId id, const ComponentInfo &component);
    void drawProperties(ObjectId id, const std::string &component, const std::vector<PropertyInfo> &properties,
        const std::string &prefix);
    // Draws the widget of one property; returns true if the value was changed.
    bool drawValue(const PropertyInfo &info, PropertyValue &value);
    void drawAddComponent(ObjectId id);
};
