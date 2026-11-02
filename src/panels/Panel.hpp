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
    // Overrides the theme's window padding, e.g. 0 for a panel showing an image edge to edge.
    void setWindowPadding(ImVec2 padding) { _windowPadding = padding; }

protected:
    // Widgets of the panel, drawn between ImGui::Begin() and ImGui::End().
    virtual void drawContent() = 0;

private:
    std::string _name;
    ImGuiWindowFlags _flags;
    ImVec2 _initialSize = ImVec2(0, 0);
    std::optional<ImVec2> _windowPadding;
    bool _visible = true;
    bool _closable = true;
    bool _focusRequested = false;
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
