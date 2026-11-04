/*
    Created on: 30/03/2026
    Filename: MenuBar.hpp
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

#include <functional>
#include <memory>
#include <string>
#include <vector>

// Main menu bar at the top of the window, built from registered Actions.
// Menus are addressed by path: "File", "View/Panels" (submenu Panels of View).
// Menus appear in the order they are first used.
class MenuBar
{
public:
    explicit MenuBar(const ActionRegistry &actions) : _actions(actions) {}

    // Throws std::invalid_argument if the action is not registered.
    void addAction(const std::string &menuPath, const std::string &actionId);
    void addSeparator(const std::string &menuPath);
    // Custom ImGui content drawn inside the menu (e.g. a dynamic list).
    void addCustom(const std::string &menuPath, std::function<void()> draw);

    // Draws the bar. Call once per frame, before the toolbar and the panels.
    void draw() const;

    void setVisible(bool visible) { _visible = visible; }
    bool isVisible() const { return _visible; }

private:
    struct Menu;
    struct Entry
    {
        enum class Type
        {
            Action,
            Separator,
            Submenu,
            Custom,
        };
        Type type;
        std::string actionId;
        Menu *submenu = nullptr;
        std::function<void()> draw;
    };
    struct Menu
    {
        std::string name;
        std::vector<Entry> entries;
        std::vector<std::unique_ptr<Menu>> children;
    };

    const ActionRegistry &_actions;
    std::vector<std::unique_ptr<Menu>> _menus;
    bool _visible = true;

    Menu &findOrCreate(const std::string &menuPath);
    void drawEntries(const Menu &menu) const;
    void drawAction(const Action &action) const;
};
