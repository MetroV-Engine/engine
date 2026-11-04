/*
    Created on: 30/03/2026
    Filename: PanelManager.cpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "PanelManager.hpp"

#include <algorithm>
#include <stdexcept>

Panel &PanelManager::registerPanel(std::string name, std::function<void()> content, ImGuiWindowFlags flags)
{
    return registerPanel<FunctionPanel>(std::move(name), std::move(content), flags);
}

void PanelManager::add(std::unique_ptr<Panel> panel)
{
    // ImGui identifies windows by name: two panels with the same name would
    // be merged into one window.
    if (hasPanel(panel->getName()))
        throw std::invalid_argument("Panel \"" + panel->getName() + "\" is already registered");
    _panels.push_back(std::move(panel));
}

Panel *PanelManager::getPanel(const std::string &name) const
{
    const auto it = std::find_if(
        _panels.begin(), _panels.end(), [&name](const auto &panel) { return panel->getName() == name; });
    return it != _panels.end() ? it->get() : nullptr;
}

bool PanelManager::unregisterPanel(const std::string &name)
{
    const auto it = std::find_if(
        _panels.begin(), _panels.end(), [&name](const auto &panel) { return panel->getName() == name; });
    if (it == _panels.end())
        return false;
    _panels.erase(it);
    return true;
}

bool PanelManager::setVisible(const std::string &name, bool visible)
{
    Panel *panel = getPanel(name);
    if (!panel)
        return false;
    panel->setVisible(visible);
    return true;
}

bool PanelManager::toggle(const std::string &name)
{
    Panel *panel = getPanel(name);
    if (!panel)
        return false;
    panel->setVisible(!panel->isVisible());
    return true;
}

void PanelManager::drawAll()
{
    for (const auto &panel : _panels)
        panel->draw();
}
