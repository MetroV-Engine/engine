/*
    Created on: 10/11/2026
    Filename: AssetBrowserPanel.hpp
    Description: Panel to browse the assets folder and drag its files onto the Inspector

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#pragma once

#include "core/AssetLibrary/AssetLibrary.hpp"
#include "panels/Panel/Panel.hpp"
#include "ui/TextInput/TextInput.hpp"

#include <cstddef>
#include <map>
#include <string>
#include <vector>

// The assets of the project (see AssetLibrary), as a tree of folders, or the
// matching files while searching. A file shows its type and size when
// selected; dragged onto a text field of the Inspector (a sprite's
// texture...), it writes its path there. The folder is read again every few
// seconds while the panel is shown.
class AssetBrowserPanel : public Panel
{
public:
    // library must outlive the panel.
    explicit AssetBrowserPanel(AssetLibrary &library);

protected:
    void drawContent() override;

private:
    // A folder of the tree: its subfolders by name, and its files (indices
    // into the library's assets).
    struct Folder
    {
        std::map<std::string, Folder> folders;
        std::vector<std::size_t> files;
    };

    AssetLibrary &_library;
    TextInput _search;
    Folder _root;
    // The library's generation the tree was built from.
    std::size_t _treeGeneration = 0;
    // Path of the selected file: it stays selected across scans.
    std::string _selected;
    double _nextScan = 0.0;

    void buildTree();
    void drawFolder(const Folder &folder);
    void drawAsset(std::size_t index, const std::string &label);
    void drawDetails() const;
};
