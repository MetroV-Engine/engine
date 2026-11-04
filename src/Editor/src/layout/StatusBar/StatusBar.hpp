/*
    Created on: 30/03/2026
    Filename: StatusBar.hpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#pragma once

#include <functional>
#include <string>
#include <utility>
#include <vector>

// Thin bar at the bottom of the window. It shows a status message on the
// left and small items on the right (FPS, theme...). Its space is reserved,
// so the dockspace and panels end above it.
class StatusBar
{
public:
    // Text on the left, e.g. "Ready" or the last notification.
    void setMessage(std::string message) { _message = std::move(message); }
    const std::string &getMessage() const { return _message; }

    // Item drawn on the right, from left to right in the order added. The
    // callback draws ImGui widgets (usually text) for the current frame.
    void addItem(std::function<void()> draw) { _items.push_back(std::move(draw)); }

    // Draws the bar. Call once per frame, after the menu bar and toolbar and
    // before the dockspace.
    void draw() const;

    void setVisible(bool visible) { _visible = visible; }
    bool isVisible() const { return _visible; }

private:
    std::string _message = "Ready";
    std::vector<std::function<void()>> _items;
    bool _visible = true;
};
