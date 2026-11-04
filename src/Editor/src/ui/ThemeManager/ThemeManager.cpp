/*
    Created on: 30/03/2026
    Filename: ThemeManager.cpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "ThemeManager.hpp"

#include <stdexcept>
#include <utility>

namespace {

const ThemeManager *activeManager = nullptr;

} // namespace

ThemeManager::ThemeManager()
{
    _themes.push_back(std::make_unique<Theme>(Theme::makeDark()));
    _themes.push_back(std::make_unique<Theme>(Theme::makeLight()));
    activeManager = this;
}

ThemeManager::~ThemeManager()
{
    if (activeManager == this)
        activeManager = nullptr;
}

void ThemeManager::add(Theme theme)
{
    for (const auto &existing : _themes)
        if (existing->name == theme.name)
            throw std::invalid_argument("Theme \"" + theme.name + "\" is already registered");
    _themes.push_back(std::make_unique<Theme>(std::move(theme)));
}

bool ThemeManager::setTheme(const std::string &name)
{
    for (std::size_t i = 0; i < _themes.size(); i++) {
        if (_themes[i]->name != name)
            continue;
        _current = i;
        apply();
        if (_onChange)
            _onChange(current());
        return true;
    }
    return false;
}

std::vector<std::string> ThemeManager::getThemeNames() const
{
    std::vector<std::string> names;
    for (const auto &theme : _themes)
        names.push_back(theme->name);
    return names;
}

void ThemeManager::setScale(float scale)
{
    _scale = scale;
    apply();
}

void ThemeManager::apply() const
{
    if (!ImGui::GetCurrentContext())
        return;
    ImGuiStyle &style = ImGui::GetStyle();
    current().applyTo(style);
    style.ScaleAllSizes(_scale);
    style.FontScaleDpi = _scale;
}

const Theme &ThemeManager::active()
{
    static const Theme fallback = Theme::makeDark();
    return activeManager ? activeManager->current() : fallback;
}
