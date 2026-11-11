/*
    Created on: 10/11/2026
    Filename: AssetBrowserPanel.cpp
    Description: Panel to browse the assets folder and drag its files onto the Inspector

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "AssetBrowserPanel.hpp"
#include "panels/AssetPayload.hpp"
#include "ui/Button/Button.hpp"

#include <imgui.h>

#include <algorithm>
#include <cctype>
#include <cfloat>
#include <cstdio>
#include <filesystem>
#include <string_view>
#include <system_error>

namespace {

// Files added or removed outside the editor show up after this delay.
constexpr double ScanSeconds = 2.0;

std::string toLower(std::string text)
{
    std::ranges::transform(text, text.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return text;
}

std::string sizeText(std::uintmax_t bytes)
{
    char text[32];
    if (bytes < 1024)
        std::snprintf(text, sizeof(text), "%ju B", bytes);
    else if (bytes < 1024 * 1024)
        std::snprintf(text, sizeof(text), "%.1f KB", static_cast<double>(bytes) / 1024.0);
    else
        std::snprintf(text, sizeof(text), "%.1f MB", static_cast<double>(bytes) / (1024.0 * 1024.0));
    return text;
}

} // namespace

AssetBrowserPanel::AssetBrowserPanel(AssetLibrary &library)
    : Panel("Assets"), _library(library), _search("##AssetSearch", "Search assets")
{
}

void AssetBrowserPanel::drawContent()
{
    if (ImGui::GetTime() >= _nextScan) {
        _library.scan();
        _nextScan = ImGui::GetTime() + ScanSeconds;
    }
    // The tree holds indices into the assets: built again after every scan,
    // this panel's or another's (the assets console command).
    if (_treeGeneration != _library.generation())
        buildTree();

    const std::filesystem::path &folder = _library.folder();
    std::error_code error;
    if (!std::filesystem::is_directory(folder, error)) {
        ImGui::TextWrapped("No assets folder yet. Images, sounds and fonts go in %s.", folder.string().c_str());
        if (Button("Create the folder").render()) {
            std::filesystem::create_directories(folder, error);
            _nextScan = 0.0;
        }
        return;
    }

    _search.setWidth(-FLT_MIN);
    _search.render();
    ImGui::Separator();

    // The list scrolls; the details of the selected file stay below it.
    const float detailsHeight = ImGui::GetTextLineHeightWithSpacing() * 2.0f + ImGui::GetFrameHeightWithSpacing();
    ImGui::BeginChild("##AssetList", ImVec2(0.0f, -detailsHeight));
    const std::string filter = toLower(_search.getValue());
    const std::vector<AssetLibrary::Asset> &assets = _library.assets();
    if (assets.empty()) {
        ImGui::TextDisabled("The folder is empty: add images, sounds or fonts to it.");
    } else if (filter.empty()) {
        drawFolder(_root);
    } else {
        for (std::size_t i = 0; i < assets.size(); i++)
            if (toLower(assets[i].path).find(filter) != std::string::npos)
                drawAsset(i, assets[i].path);
    }
    if (_library.truncated())
        ImGui::TextDisabled("Only the first %zu files are listed.", AssetLibrary::MaxAssets);
    ImGui::EndChild();
    drawDetails();
}

void AssetBrowserPanel::buildTree()
{
    _treeGeneration = _library.generation();
    _root = {};
    const std::vector<AssetLibrary::Asset> &assets = _library.assets();
    for (std::size_t i = 0; i < assets.size(); i++) {
        Folder *folder = &_root;
        std::string_view path = assets[i].path;
        for (std::size_t slash = path.find('/'); slash != std::string_view::npos; slash = path.find('/')) {
            folder = &folder->folders[std::string(path.substr(0, slash))];
            path.remove_prefix(slash + 1);
        }
        folder->files.push_back(i);
    }
}

void AssetBrowserPanel::drawFolder(const Folder &folder)
{
    for (const auto &[name, subfolder] : folder.folders) {
        if (ImGui::TreeNodeEx(name.c_str(), ImGuiTreeNodeFlags_SpanAvailWidth)) {
            drawFolder(subfolder);
            ImGui::TreePop();
        }
    }
    for (const std::size_t index : folder.files) {
        const std::string &path = _library.assets()[index].path;
        drawAsset(index, path.substr(path.rfind('/') + 1));
    }
}

void AssetBrowserPanel::drawAsset(std::size_t index, const std::string &label)
{
    const AssetLibrary::Asset &asset = _library.assets()[index];
    ImGui::PushID(asset.path.c_str());
    if (ImGui::Selectable(label.c_str(), asset.path == _selected))
        _selected = asset.path;
    dragAsset(asset.path);
    ImGui::SetItemTooltip("%s: drag it onto a text field of the Inspector", AssetLibrary::typeName(asset.type));
    ImGui::PopID();
}

void AssetBrowserPanel::drawDetails() const
{
    ImGui::Separator();
    const std::vector<AssetLibrary::Asset> &assets = _library.assets();
    const auto selected = std::ranges::find(assets, _selected, &AssetLibrary::Asset::path);
    if (selected == assets.end()) {
        ImGui::TextDisabled("Select a file to see its details.");
        return;
    }
    ImGui::TextUnformatted(selected->path.c_str());
    ImGui::TextDisabled("%s, %s", AssetLibrary::typeName(selected->type), sizeText(selected->size).c_str());
    ImGui::SameLine();
    if (ImGui::SmallButton("Copy path"))
        ImGui::SetClipboardText(selected->path.c_str());
}
