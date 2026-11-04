/*
    Created on: 30/03/2026
    Filename: MenuBar.cpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "MenuBar.hpp"

#include <stdexcept>
#include <utility>

namespace {

std::vector<std::string> splitPath(const std::string &path)
{
    std::vector<std::string> parts;
    std::size_t start = 0;
    while (start <= path.size()) {
        const std::size_t end = path.find('/', start);
        const std::string part = path.substr(start, end == std::string::npos ? std::string::npos : end - start);
        if (!part.empty())
            parts.push_back(part);
        if (end == std::string::npos)
            break;
        start = end + 1;
    }
    if (parts.empty())
        throw std::invalid_argument("Menu path must not be empty");
    return parts;
}

} // namespace

MenuBar::Menu &MenuBar::findOrCreate(const std::string &menuPath)
{
    std::vector<std::unique_ptr<Menu>> *level = &_menus;
    Menu *parent = nullptr;
    for (const std::string &name : splitPath(menuPath)) {
        Menu *found = nullptr;
        for (const auto &menu : *level)
            if (menu->name == name)
                found = menu.get();
        if (!found) {
            level->push_back(std::make_unique<Menu>());
            found = level->back().get();
            found->name = name;
            // A submenu is drawn where it was first declared among the parent's entries.
            if (parent)
                parent->entries.push_back({Entry::Type::Submenu, "", found, {}});
        }
        parent = found;
        level = &found->children;
    }
    return *parent;
}

void MenuBar::addAction(const std::string &menuPath, const std::string &actionId)
{
    if (!_actions.has(actionId))
        throw std::invalid_argument("Action \"" + actionId + "\" is not registered");
    findOrCreate(menuPath).entries.push_back({Entry::Type::Action, actionId, nullptr, {}});
}

void MenuBar::addSeparator(const std::string &menuPath)
{
    findOrCreate(menuPath).entries.push_back({Entry::Type::Separator, "", nullptr, {}});
}

void MenuBar::addCustom(const std::string &menuPath, std::function<void()> draw)
{
    findOrCreate(menuPath).entries.push_back({Entry::Type::Custom, "", nullptr, std::move(draw)});
}

void MenuBar::draw() const
{
    if (!_visible || !ImGui::BeginMainMenuBar())
        return;
    for (const auto &menu : _menus) {
        if (ImGui::BeginMenu(menu->name.c_str())) {
            drawEntries(*menu);
            ImGui::EndMenu();
        }
    }
    ImGui::EndMainMenuBar();
}

void MenuBar::drawEntries(const Menu &menu) const
{
    for (const Entry &entry : menu.entries) {
        switch (entry.type) {
        case Entry::Type::Action:
            if (const Action *action = _actions.get(entry.actionId))
                drawAction(*action);
            break;
        case Entry::Type::Separator:
            ImGui::Separator();
            break;
        case Entry::Type::Submenu:
            if (ImGui::BeginMenu(entry.submenu->name.c_str())) {
                drawEntries(*entry.submenu);
                ImGui::EndMenu();
            }
            break;
        case Entry::Type::Custom:
            if (entry.draw)
                entry.draw();
            break;
        }
    }
}

void MenuBar::drawAction(const Action &action) const
{
    const std::string shortcut = _actions.shortcutLabel(action);
    // Action ids keep items unique even if two actions share a label.
    const std::string label = action.label + "##" + action.id;
    if (ImGui::MenuItem(label.c_str(), shortcut.empty() ? nullptr : shortcut.c_str(), action.checked(),
            action.enabled())
        && action.callback)
        action.callback();
}
