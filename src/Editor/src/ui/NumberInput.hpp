/*
    Created on: 30/03/2026
    Filename: NumberInput.hpp
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

#include <algorithm>
#include <functional>
#include <limits>
#include <string>
#include <type_traits>
#include <utility>

// Reusable ImGui numeric field for int, float or double: label, +/- step,
// optional min/max clamping, width, enabled state and a change callback.
template <typename T>
class NumberInput
{
    static_assert(std::is_same_v<T, int> || std::is_same_v<T, float> || std::is_same_v<T, double>,
        "NumberInput supports int, float and double");

public:
    using OnChange = std::function<void(T)>;

    explicit NumberInput(std::string label, T value = T{}, T step = T{1}, OnChange onChange = {})
        : _label(std::move(label)), _value(value), _step(step), _onChange(std::move(onChange))
    {
    }

    // Draws the field; returns true on the frame the value changes.
    bool render()
    {
        if (_width != 0.0f)
            ImGui::SetNextItemWidth(_width);
        ImGui::BeginDisabled(!_enabled);

        T value = _value;
        ImGui::InputScalar(_label.c_str(), dataType(), &value, _step != T{} ? &_step : nullptr, nullptr, format());
        value = std::clamp(value, _min, _max);
        const bool changed = value != _value;
        _value = value;

        ImGui::EndDisabled();

        if (changed && _onChange)
            _onChange(_value);
        return changed;
    }

    void setValue(T value) { _value = std::clamp(value, _min, _max); }
    void setRange(T min, T max)
    {
        _min = min;
        _max = max;
        _value = std::clamp(_value, _min, _max);
    }
    void setStep(T step) { _step = step; }
    void setLabel(std::string label) { _label = std::move(label); }
    void setOnChange(OnChange onChange) { _onChange = std::move(onChange); }
    void setWidth(float width) { _width = width; }
    void setEnabled(bool enabled) { _enabled = enabled; }

    T getValue() const { return _value; }
    const std::string &getLabel() const { return _label; }
    bool isEnabled() const { return _enabled; }

private:
    std::string _label;
    T _value;
    T _step; // 0 hides the +/- buttons
    T _min = std::numeric_limits<T>::lowest();
    T _max = std::numeric_limits<T>::max();
    OnChange _onChange;
    float _width = 0.0f; // 0 = ImGui default, < 0 = fill the available width minus abs(value)
    bool _enabled = true;

    static constexpr ImGuiDataType dataType()
    {
        if constexpr (std::is_same_v<T, int>)
            return ImGuiDataType_S32;
        else if constexpr (std::is_same_v<T, float>)
            return ImGuiDataType_Float;
        else
            return ImGuiDataType_Double;
    }

    static constexpr const char *format()
    {
        if constexpr (std::is_same_v<T, int>)
            return "%d";
        else
            return "%.3f";
    }
};
