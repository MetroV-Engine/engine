/*
    Created on: 30/03/2026
    Filename: ComboBox.hpp
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

// Reusable ImGui dropdown to pick one option from a list: label, options,
// selected index, width, enabled state and a change callback.
class ComboBox
{
public:
    // Receives the selected index and the matching option.
    using OnChange = std::function<void(int, const std::string &)>;

    explicit ComboBox(std::string label, std::vector<std::string> options, int selected = 0, OnChange onChange = {});

    // Draws the dropdown; returns true on the frame the selection changes.
    bool render();

    // Out-of-range indices are ignored.
    void setSelected(int index);
    // Keeps the current selection when still valid, otherwise selects the first option.
    void setOptions(std::vector<std::string> options);
    void setLabel(std::string label) { _label = std::move(label); }
    void setOnChange(OnChange onChange) { _onChange = std::move(onChange); }
    void setWidth(float width) { _width = width; }
    void setEnabled(bool enabled) { _enabled = enabled; }

    // -1 when there are no options.
    int getSelected() const { return _selected; }
    // Empty string when there are no options.
    const std::string &getSelectedOption() const;
    const std::vector<std::string> &getOptions() const { return _options; }
    const std::string &getLabel() const { return _label; }
    bool isEnabled() const { return _enabled; }

private:
    std::string _label;
    std::vector<std::string> _options;
    int _selected = -1;
    OnChange _onChange;
    float _width = 0.0f; // 0 = ImGui default width
    bool _enabled = true;
};
