/*
    Created on: 30/03/2026
    Filename: Button.hpp
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
#include <string>
#include <utility>

// Reusable ImGui button: label, size, visual variant, enabled state and a
// click callback, configured once and drawn every frame with render().
class Button
{
public:
    enum class Variant
    {
        Primary,
        Secondary,
        Danger,
    };

    using OnClick = std::function<void()>;

    explicit Button(std::string label, OnClick onClick = {}, Variant variant = Variant::Primary,
        ImVec2 size = ImVec2(0, 0));

    // Draws the button; returns true on the frame it is clicked.
    bool render() const;

    void setLabel(std::string label) { _label = std::move(label); }
    void setOnClick(OnClick onClick) { _onClick = std::move(onClick); }
    void setVariant(Variant variant) { _variant = variant; }
    void setSize(ImVec2 size) { _size = size; }
    void setEnabled(bool enabled) { _enabled = enabled; }

    const std::string &getLabel() const { return _label; }
    bool isEnabled() const { return _enabled; }

private:
    std::string _label;
    OnClick _onClick;
    Variant _variant;
    ImVec2 _size;
    bool _enabled = true;
};
