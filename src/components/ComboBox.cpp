/*
    Created on: 30/03/2026
    Filename: ComboBox.cpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "ComboBox.hpp"

#include <imgui.h>

namespace {

const std::string emptyOption;

} // namespace

ComboBox::ComboBox(std::string label, std::vector<std::string> options, int selected, OnChange onChange)
    : _label(std::move(label)), _options(std::move(options)), _onChange(std::move(onChange))
{
    if (!_options.empty())
        _selected = 0;
    setSelected(selected);
}

bool ComboBox::render()
{
    if (_width > 0.0f)
        ImGui::SetNextItemWidth(_width);
    ImGui::BeginDisabled(!_enabled);

    bool changed = false;
    if (ImGui::BeginCombo(_label.c_str(), getSelectedOption().c_str())) {
        for (int i = 0; i < static_cast<int>(_options.size()); i++) {
            const bool isSelected = i == _selected;
            ImGui::PushID(i);
            if (ImGui::Selectable(_options[i].c_str(), isSelected) && !isSelected) {
                _selected = i;
                changed = true;
            }
            if (isSelected)
                ImGui::SetItemDefaultFocus();
            ImGui::PopID();
        }
        ImGui::EndCombo();
    }

    ImGui::EndDisabled();

    if (changed && _onChange)
        _onChange(_selected, _options[_selected]);
    return changed;
}

void ComboBox::setSelected(int index)
{
    if (index >= 0 && index < static_cast<int>(_options.size()))
        _selected = index;
}

void ComboBox::setOptions(std::vector<std::string> options)
{
    _options = std::move(options);
    if (_options.empty())
        _selected = -1;
    else if (_selected < 0 || _selected >= static_cast<int>(_options.size()))
        _selected = 0;
}

const std::string &ComboBox::getSelectedOption() const
{
    return _selected >= 0 ? _options[_selected] : emptyOption;
}
