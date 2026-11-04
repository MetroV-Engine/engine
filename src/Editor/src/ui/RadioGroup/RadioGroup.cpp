/*
    Created on: 30/03/2026
    Filename: RadioGroup.cpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "RadioGroup.hpp"

#include <imgui.h>

namespace {

const std::string emptyOption;

} // namespace

RadioGroup::RadioGroup(std::string label, std::vector<std::string> options, int selected, OnChange onChange)
    : _label(std::move(label)), _options(std::move(options)), _onChange(std::move(onChange))
{
    if (!_options.empty())
        _selected = 0;
    setSelected(selected);
}

bool RadioGroup::render()
{
    ImGui::BeginDisabled(!_enabled);
    // Scopes the option IDs so two groups can share option names.
    ImGui::PushID(_label.c_str());

    // Like other ImGui widgets, text after "##" is part of the ID but not displayed.
    const std::string visibleLabel = _label.substr(0, _label.find("##"));
    if (!visibleLabel.empty())
        ImGui::TextUnformatted(visibleLabel.c_str());

    bool changed = false;
    for (int i = 0; i < static_cast<int>(_options.size()); i++) {
        if (_horizontal && i > 0)
            ImGui::SameLine();
        ImGui::PushID(i);
        if (ImGui::RadioButton(_options[i].c_str(), i == _selected) && i != _selected) {
            _selected = i;
            changed = true;
        }
        ImGui::PopID();
    }

    ImGui::PopID();
    ImGui::EndDisabled();

    if (changed && _onChange)
        _onChange(_selected, _options[_selected]);
    return changed;
}

void RadioGroup::setSelected(int index)
{
    if (index >= 0 && index < static_cast<int>(_options.size()))
        _selected = index;
}

void RadioGroup::setOptions(std::vector<std::string> options)
{
    _options = std::move(options);
    if (_options.empty())
        _selected = -1;
    else if (_selected < 0 || _selected >= static_cast<int>(_options.size()))
        _selected = 0;
}

const std::string &RadioGroup::getSelectedOption() const
{
    return _selected >= 0 ? _options[_selected] : emptyOption;
}
