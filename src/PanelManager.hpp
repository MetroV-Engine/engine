/*
    Created on: 30/03/2026
    Filename: PanelManager.hpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#pragma once

#include "panels/Panel.hpp"

#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

// Central registry of the editor panels. Panels are registered once, then
// drawAll() draws every visible one each frame, in registration order. Menu
// and toolbar entries to show/hide them are created through Actions.
class PanelManager
{
public:
    // Creates and registers a panel of type T (derived from Panel). The
    // manager owns it; the returned reference stays valid for its lifetime.
    // Throws std::invalid_argument if a panel with the same name exists.
    template <typename T, typename... Args>
    T &registerPanel(Args &&...args)
    {
        auto panel = std::make_unique<T>(std::forward<Args>(args)...);
        T &ref = *panel;
        add(std::move(panel));
        return ref;
    }

    // Registers a panel drawn by a simple callback.
    Panel &registerPanel(std::string name, std::function<void()> content,
        ImGuiWindowFlags flags = ImGuiWindowFlags_None);

    // Returns nullptr if no panel has this name.
    Panel *getPanel(const std::string &name) const;
    bool hasPanel(const std::string &name) const { return getPanel(name) != nullptr; }
    // Returns false if no panel has this name. Must not be called from a
    // panel's drawContent() (it would destroy the panel being drawn).
    bool unregisterPanel(const std::string &name);

    // Returns false if no panel has this name.
    bool setVisible(const std::string &name, bool visible);
    bool toggle(const std::string &name);

    // Draws every visible panel. Call once per frame, after the menu bar and
    // toolbar so panels are laid out below them.
    void drawAll();

    const std::vector<std::unique_ptr<Panel>> &getPanels() const { return _panels; }

private:
    std::vector<std::unique_ptr<Panel>> _panels;

    void add(std::unique_ptr<Panel> panel);
};
