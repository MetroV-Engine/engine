/*
    Created on: 30/03/2026
    Filename: RadioGroup.hpp
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

// Reusable group of ImGui radio buttons to pick one option from a short,
// always-visible list: label, options, layout, enabled state and a change callback.
class RadioGroup
{
public:
    // Receives the selected index and the matching option.
    using OnChange = std::function<void(int, const std::string &)>;

    explicit RadioGroup(std::string label, std::vector<std::string> options, int selected = 0, OnChange onChange = {});

    // Draws the group; returns true on the frame the selection changes.
    bool render();

    // Out-of-range indices are ignored.
    void setSelected(int index);
    // Keeps the current selection when still valid, otherwise selects the first option.
    void setOptions(std::vector<std::string> options);
    void setLabel(std::string label) { _label = std::move(label); }
    void setOnChange(OnChange onChange) { _onChange = std::move(onChange); }
    // Lays the options out on one line instead of one per line.
    void setHorizontal(bool horizontal) { _horizontal = horizontal; }
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
    bool _horizontal = false;
    bool _enabled = true;
};
