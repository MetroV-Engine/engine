/*
    Created on: 30/03/2026
    Filename: Panel.hpp
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

#include <functional>
#include <optional>
#include <string>
#include <utility>

// Base class of an editor panel: an ImGui window with a unique name, a
// visibility flag and its own content. Subclasses only implement drawContent();
// the PanelManager opens/closes the window around it.
class Panel
{
public:
    explicit Panel(std::string name, ImGuiWindowFlags flags = ImGuiWindowFlags_None);
    virtual ~Panel() = default;

    Panel(const Panel &) = delete;
    Panel &operator=(const Panel &) = delete;

    // Draws the window and its content if visible. Called by the PanelManager.
    // Override only for panels that create their own ImGui window.
    virtual void draw();

    const std::string &getName() const { return _name; }
    bool isVisible() const { return _visible; }
    // Showing a hidden panel also brings it to the front (selects its tab if docked).
    void setVisible(bool visible)
    {
        _focusRequested = visible && !_visible;
        _visible = visible;
    }
    // Whether the panel has a close button and appears in the View menu.
    bool isClosable() const { return _closable; }
    void setClosable(bool closable) { _closable = closable; }
    // Size used the first time the panel appears (0 = fit content).
    void setInitialSize(ImVec2 size) { _initialSize = size; }
    // When docked in the editor, shows no tab: the panel looks like a fixed
    // bar. Such a panel should draw a drawMoveGrip() to drag it out; once
    // floating it has a normal title bar, to dock it somewhere else.
    void setHideTabBar(bool hide) { _hideTabBar = hide; }
    // Keeps the panel where the default layout docks it: it cannot be
    // undocked, closed, or have other panels docked over it as tabs. It can
    // still be resized with the dock splitters.
    void setLocked(bool locked)
    {
        _locked = locked;
        if (locked)
            _closable = false;
    }
    // Called by the grip's "Reset Position" (e.g. to restore the default layout).
    void setOnResetPosition(std::function<void()> callback) { _onResetPosition = std::move(callback); }
    // Overrides the theme's window padding, e.g. 0 for a panel showing an image edge to edge.
    void setWindowPadding(ImVec2 padding) { _windowPadding = padding; }

protected:
    // Widgets of the panel, drawn between ImGui::Begin() and ImGui::End().
    virtual void drawContent() = 0;
    // Smallest size of the window when floating (0 = no minimum). Docked, the
    // dock area decides.
    virtual ImVec2 minimumSize() const { return ImVec2(0.0f, 0.0f); }

    // Handle (a block of dots) to drag a panel that has no tab: dragging it
    // undocks the panel and moves it; dropped on a dock area it docks again.
    // Right click: undock, or put it back in the default layout.
    void drawMoveGrip(float height);


private:
    std::string _name;
    ImGuiWindowFlags _flags;
    ImVec2 _initialSize = ImVec2(0, 0);
    std::optional<ImVec2> _windowPadding;
    bool _visible = true;
    bool _closable = true;
    bool _focusRequested = false;
    bool _hideTabBar = false;
    bool _locked = false;
    std::function<void()> _onResetPosition;
};

// Panel whose content is a callback, for small panels that do not need a class.
class FunctionPanel : public Panel
{
public:
    FunctionPanel(std::string name, std::function<void()> content, ImGuiWindowFlags flags = ImGuiWindowFlags_None)
        : Panel(std::move(name), flags), _content(std::move(content))
    {
    }

protected:
    void drawContent() override
    {
        if (_content)
            _content();
    }

private:
    std::function<void()> _content;
};
