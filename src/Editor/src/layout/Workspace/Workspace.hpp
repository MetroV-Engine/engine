/*
    Created on: 30/03/2026
    Filename: Workspace.hpp
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
#include <map>
#include <string>

// The user's workspace preferences (window geometry, open panels, theme,
// console filters...), saved between launches in a "[Workspace][Editor]"
// section of ImGui's ini file, next to the panel layout. ImGui writes the file
// itself, a few seconds after a change and when the editor closes.
//
// The editor provides two functions: collect() writes its current state into
// key/value pairs, apply() restores it from them. Values are plain strings;
// the helpers below read them with a fallback.
class Workspace
{
public:
    using Values = std::map<std::string, std::string>;

    // Registers the ini section. Requires an ImGui context, and must be
    // created before the first frame (ImGui reads the file then).
    Workspace(std::function<void(Values &)> collect, std::function<void(const Values &)> apply);
    // Saves now and stops ImGui from saving again without this section.
    ~Workspace();

    Workspace(const Workspace &) = delete;
    Workspace &operator=(const Workspace &) = delete;

    // Reads the ini file now (layout included) and applies the saved values.
    // Returns false if there was no saved workspace.
    bool load();
    // Writes the ini file now.
    void save() const;
    // Call once per frame: tells ImGui to save when a value changed.
    void update();

    static bool getBool(const Values &values, const std::string &key, bool fallback);
    static int getInt(const Values &values, const std::string &key, int fallback);
    static std::string getString(const Values &values, const std::string &key, const std::string &fallback);
    static void setBool(Values &values, const std::string &key, bool value) { values[key] = value ? "1" : "0"; }
    static void setInt(Values &values, const std::string &key, int value) { values[key] = std::to_string(value); }

private:
    std::function<void(Values &)> _collect;
    std::function<void(const Values &)> _apply;
    // Values read from the file, filled by the ini handler.
    Values _read;
    bool _found = false;
    // Last state known to be saved, to detect changes.
    Values _saved;
    double _nextCheck = 0.0;

    void write(std::string &out) const;
};
