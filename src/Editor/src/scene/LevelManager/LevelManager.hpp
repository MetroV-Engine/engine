/*
    Created on: 10/11/2026
    Filename: LevelManager.hpp
    Description: The levels of the project: files opened into the scene and saved from it

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#pragma once

#include "scene/HierarchyModel.hpp"
#include "scene/PropertyModel.hpp"

#include <filesystem>
#include <string>
#include <vector>

// The levels of the project: "<name>.level" files in a folder ("levels",
// next to imgui.ini by default), opened into the scene and saved from it
// with LevelFile. A refused change throws std::runtime_error with a message
// meant for the user (a name already taken, a file that cannot be written...).
class LevelManager
{
public:
    static constexpr const char *Extension = ".level";

    // The scene must outlive the manager. A relative folder is taken from
    // the working directory at construction.
    LevelManager(HierarchyModel &hierarchy, PropertyModel &properties, const std::filesystem::path &folder = "levels");

    // Names of the levels in the folder, sorted.
    std::vector<std::string> list() const;
    // The level opened or saved last; empty if none.
    const std::string &current() const { return _current; }
    // True if the scene differs from the current level's file or, without a
    // current level, if it has objects.
    bool hasUnsavedChanges() const;

    // Empties the scene and saves it as a new level, which becomes current.
    void create(const std::string &name);
    // Replaces the scene by the level; returns what the scene refused.
    std::vector<std::string> open(const std::string &name);
    // Saves the scene in the current level.
    void save();
    // Saves the scene as a new level, which becomes current.
    void saveAs(const std::string &name);
    void rename(const std::string &from, const std::string &to);
    // Deletes the file; the scene stays, as unsaved if it was this level.
    void remove(const std::string &name);

    // Letters, digits, - and _ only: names are file names.
    static bool isValidName(const std::string &name);

private:
    HierarchyModel &_hierarchy;
    PropertyModel &_properties;
    std::filesystem::path _folder;
    std::string _current;
    // The current level as last saved or opened, to detect unsaved changes.
    std::string _savedText;

    std::filesystem::path pathOf(const std::string &name) const;
    // Throws unless name is valid and free.
    void checkNew(const std::string &name) const;
    void checkExists(const std::string &name) const;
    // Writes the scene in the level's file, which becomes current.
    void write(const std::string &name);
};
