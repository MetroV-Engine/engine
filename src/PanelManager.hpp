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
// drawAll() draws every visible one each frame, in registration order, with a
// main menu bar whose View menu shows/hides them.
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

    // Draws the main menu bar and every visible panel. Call once per frame,
    // between beginFrame() and present().
    void drawAll();

    // Extra menus drawn in the main menu bar, before View (e.g. File).
    void setMenuBarCallback(std::function<void()> callback) { _menuBarCallback = std::move(callback); }
    void setMenuBarVisible(bool visible) { _menuBarVisible = visible; }

    const std::vector<std::unique_ptr<Panel>> &getPanels() const { return _panels; }

private:
    std::vector<std::unique_ptr<Panel>> _panels;
    std::function<void()> _menuBarCallback;
    bool _menuBarVisible = true;

    void add(std::unique_ptr<Panel> panel);
    void drawMenuBar();
};
