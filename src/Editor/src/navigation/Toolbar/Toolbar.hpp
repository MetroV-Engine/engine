/*
    Created on: 30/03/2026
    Filename: Toolbar.hpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#pragma once

#include "navigation/ActionRegistry/ActionRegistry.hpp"

#include <string>
#include <vector>

// Horizontal bar of buttons under the menu bar, each one triggering an Action.
// The space it takes is reserved, so panels and overlays are laid out below it.
class Toolbar
{
public:
    explicit Toolbar(const ActionRegistry &actions) : _actions(actions) {}

    // Throws std::invalid_argument if the action is not registered.
    void addAction(const std::string &actionId);
    void addSeparator();

    // Draws the bar. Call once per frame, after the menu bar and before the panels.
    void draw() const;

    void setVisible(bool visible) { _visible = visible; }
    bool isVisible() const { return _visible; }

private:
    const ActionRegistry &_actions;
    // An empty id is a separator.
    std::vector<std::string> _items;
    bool _visible = true;

    void drawButton(const Action &action) const;
};
