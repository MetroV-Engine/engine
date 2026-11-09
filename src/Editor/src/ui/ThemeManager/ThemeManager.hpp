/*
    Created on: 30/03/2026
    Filename: ThemeManager.hpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#pragma once

#include "ui/Theme/Theme.hpp"

#include <functional>
#include <memory>
#include <string>
#include <vector>

// Holds the available themes and applies the selected one to ImGui, scaled
// to the display. Components read the active theme through ThemeManager::active().
class ThemeManager
{
public:
    // Registers the built-in themes (Dark, Light) and applies Dark.
    ThemeManager();
    ~ThemeManager();

    ThemeManager(const ThemeManager &) = delete;
    ThemeManager &operator=(const ThemeManager &) = delete;

    // Throws std::invalid_argument if a theme with this name exists.
    void add(Theme theme);
    // Returns false if no theme has this name.
    bool setTheme(const std::string &name);
    const Theme &current() const { return *_themes[_current]; }
    std::vector<std::string> getThemeNames() const;

    // DPI scale applied on top of the theme metrics.
    void setScale(float scale);
    float getScale() const { return _scale; }

    // Called after a theme change (e.g. to update a settings widget).
    void setOnChange(std::function<void(const Theme &)> onChange) { _onChange = std::move(onChange); }

    // Theme of the running editor, for components; the Dark theme if no
    // ThemeManager exists.
    static const Theme &active();

private:
    std::vector<std::unique_ptr<Theme>> _themes;
    std::size_t _current = 0;
    float _scale = 1.0f;
    std::function<void(const Theme &)> _onChange;

    void apply() const;
};
