/*
    Created on: 10/11/2026
    Filename: AssetLibrary.hpp
    Description: The resources of the project: the files of the assets folder

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

// The resources of the project: the files of a folder ("assets", next to
// imgui.ini by default) and of its subfolders, typed by their extension.
// Their paths, relative to the folder, are what components hold (a sprite's
// texture...). Hidden files and folders (starting with '.') are skipped.
class AssetLibrary
{
public:
    enum class Type
    {
        Image,
        Audio,
        Font,
        Level,
        Text,
        Other,
    };

    struct Asset
    {
        // Relative to the folder, with '/' separators: "sprites/hero.png".
        std::string path;
        Type type;
        std::uintmax_t size;
    };

    // A scan stops there, so a huge folder cannot freeze the editor.
    static constexpr std::size_t MaxAssets = 10000;

    // A relative folder is taken from the working directory at construction.
    explicit AssetLibrary(const std::filesystem::path &folder = "assets");

    const std::filesystem::path &folder() const { return _folder; }
    // Reads the folder again.
    void scan();
    // Sorted by path; empty if the folder does not exist.
    const std::vector<Asset> &assets() const { return _assets; }
    // True if the last scan stopped at MaxAssets.
    bool truncated() const { return _truncated; }

    static Type typeOf(const std::filesystem::path &file);
    static const char *typeName(Type type);

private:
    std::filesystem::path _folder;
    std::vector<Asset> _assets;
    bool _truncated = false;
};
