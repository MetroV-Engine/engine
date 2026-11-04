/*
    Created on: 30/03/2026
    Filename: TextInput.hpp
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

// Reusable ImGui text field: label, placeholder, width, password mode,
// enabled state and callbacks on change and on Enter.
class TextInput
{
public:
    using OnChange = std::function<void(const std::string &)>;

    explicit TextInput(std::string label, std::string placeholder = "", OnChange onChange = {});

    // Draws the field; returns true on the frame the value changes.
    bool render();

    void setValue(std::string value) { _value = std::move(value); }
    void setLabel(std::string label) { _label = std::move(label); }
    void setPlaceholder(std::string placeholder) { _placeholder = std::move(placeholder); }
    void setOnChange(OnChange onChange) { _onChange = std::move(onChange); }
    void setOnSubmit(OnChange onSubmit) { _onSubmit = std::move(onSubmit); }
    void setWidth(float width) { _width = width; }
    void setPassword(bool password) { _password = password; }
    void setEnabled(bool enabled) { _enabled = enabled; }

    const std::string &getValue() const { return _value; }
    const std::string &getLabel() const { return _label; }
    bool isEnabled() const { return _enabled; }

private:
    std::string _label;
    std::string _placeholder;
    std::string _value;
    OnChange _onChange;
    OnChange _onSubmit;
    float _width = 0.0f; // 0 = ImGui default, < 0 = fill the available width minus abs(value)
    bool _password = false;
    bool _enabled = true;
};
