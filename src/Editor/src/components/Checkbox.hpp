/*
    Created on: 30/03/2026
    Filename: Checkbox.hpp
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

// Reusable ImGui checkbox for a boolean value: label, enabled state and a
// change callback.
class Checkbox
{
public:
    using OnChange = std::function<void(bool)>;

    explicit Checkbox(std::string label, bool checked = false, OnChange onChange = {});

    // Draws the checkbox; returns true on the frame the value changes.
    bool render();

    void setChecked(bool checked) { _checked = checked; }
    void setLabel(std::string label) { _label = std::move(label); }
    void setOnChange(OnChange onChange) { _onChange = std::move(onChange); }
    void setEnabled(bool enabled) { _enabled = enabled; }

    bool isChecked() const { return _checked; }
    const std::string &getLabel() const { return _label; }
    bool isEnabled() const { return _enabled; }

private:
    std::string _label;
    bool _checked;
    OnChange _onChange;
    bool _enabled = true;
};
