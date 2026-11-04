/*
    Created on: 30/03/2026
    Filename: CommandPalette.hpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#pragma once

#include "navigation/ActionRegistry/ActionRegistry.hpp"

#include <string>
#include <vector>

// Searchable list of every editor command (Ctrl+Shift+P): type a few letters
// ("fsc" finds "View: Fullscreen"), move with Up/Down, run with Enter.
// Recently used commands are listed first.
class CommandPalette
{
public:
    explicit CommandPalette(ActionRegistry &actions) : _actions(actions) {}

    // Opens the palette, or closes it if it is open. Takes effect in draw().
    void toggle() { _toggleRequested = true; }
    bool isOpen() const { return _open; }

    // Draws the palette when open. Call once per frame, after the panels.
    void draw();

    // How well query matches text, or -1 if it does not: every query letter
    // must appear in order; matches at word starts and in a row score higher.
    static int fuzzyScore(const std::string &query, const std::string &text);
    // Text shown and searched for an action: "<Category>: <label>".
    static std::string displayName(const Action &action);

private:
    static constexpr std::size_t MaxRecent = 5;

    ActionRegistry &_actions;
    bool _toggleRequested = false;
    bool _open = false;
    std::string _query;
    int _selected = 0;
    bool _scrollToSelected = false;
    std::vector<std::string> _recent; // Action ids, most recent first

    // Actions matching the query, best first.
    std::vector<const Action *> matches() const;
    void run(const std::string &id);
};
