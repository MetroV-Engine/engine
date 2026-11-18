/*
    Created on: 30/03/2026
    Filename: EditorLayout.hpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#pragma once

#include <imgui.h>

#include <string>
#include <utility>
#include <vector>

// Main editor layout: a dockspace filling the window below the menu bar and
// toolbar (and above the status bar), where panels are docked as tabs and
// splits. Users can rearrange it freely; ImGui saves it in imgui.ini. The
// default arrangement is built on first launch and by resetLayout().
//
//   +-----------+---------------------------+-------------+
//   |           |                           |             |
//   |   Left    |          Center           |    Right    |
//   |           |                           |             |
//   +-----------+---------------------------+             |
//   |   Left    |         Controls          |             |
//   |  Bottom   |          Bottom           |             |
//   +-----------+---------------------------+-------------+
class EditorLayout
{
public:
    enum class Area
    {
        Left,
        // Below Left (e.g. a palette to drag from, which must stay visible
        // next to the Hierarchy).
        LeftBottom,
        Center,
        Right,
        Bottom,
        // Thin strip right above Bottom (e.g. the game controls).
        Controls,
    };

    // Enables docking in ImGui. Requires an ImGui context (create after GuiContext).
    EditorLayout();

    // Where a panel goes in the default layout. Panels sharing an area become
    // tabs, the first one being selected.
    void setDefaultArea(const std::string &panelName, Area area);

    // Rebuilds the default layout on the next frame.
    void resetLayout() { _resetRequested = true; }

    // Draws the dockspace. Call once per frame, after the menu bar, toolbar
    // and status bar (they reserve their space first) and before the panels.
    void draw();

    // Fractions of the window used by the side areas in the default layout.
    void setAreaRatios(float left, float right, float bottom);

private:
    std::vector<std::pair<std::string, Area>> _defaultAreas;
    bool _resetRequested = false;
    // Whether the first frame placed the panels a saved layout did not know.
    bool _newPanelsDocked = false;
    // Frames to wait before selecting each area's first tab (its window must exist).
    int _selectFirstTabsIn = 0;
    float _leftRatio = 0.18f;
    float _rightRatio = 0.24f;
    float _bottomRatio = 0.28f;

    void buildDefaultLayout(ImGuiID dockspaceId, ImVec2 size);
    // A layout saved before a panel existed would show it floating: it joins
    // a panel of its area instead, or below the Left area for LeftBottom, as
    // in the default layout.
    void dockNewPanels(ImGuiID dockspaceId);
    // The dock node of a panel of this area in the saved layout, or 0.
    ImGuiID savedNode(Area area) const;
    void selectFirstTabs() const;
};
