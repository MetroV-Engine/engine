/*
    Created on: 10/11/2026
    Filename: AssetLibrary.cpp
    Description: The resources of the project: the files of the assets folder

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "AssetLibrary.hpp"

#include <algorithm>
#include <cctype>
#include <initializer_list>
#include <system_error>

AssetLibrary::AssetLibrary(const std::filesystem::path &folder) : _folder(std::filesystem::absolute(folder)) {}

void AssetLibrary::scan()
{
    ++_generation;
    _assets.clear();
    _truncated = false;
    using Iterator = std::filesystem::recursive_directory_iterator;
    std::error_code error;
    for (Iterator it(_folder, std::filesystem::directory_options::skip_permission_denied, error); !error && it != Iterator();
         it.increment(error)) {
        // Checking an entry may fail (a broken link): that entry is skipped, not the scan.
        std::error_code ignored;
        const std::string name = it->path().filename().string();
        if (!name.empty() && name.front() == '.') {
            if (it->is_directory(ignored))
                it.disable_recursion_pending();
            continue;
        }
        if (!it->is_regular_file(ignored))
            continue;
        if (_assets.size() == MaxAssets) {
            _truncated = true;
            break;
        }
        const std::uintmax_t size = it->file_size(ignored);
        _assets.push_back({it->path().lexically_relative(_folder).generic_string(), typeOf(it->path()), ignored ? 0 : size});
    }
    std::ranges::sort(_assets, {}, &Asset::path);
}

AssetLibrary::Type AssetLibrary::typeOf(const std::filesystem::path &file)
{
    std::string extension = file.extension().string();
    std::ranges::transform(extension, extension.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    const auto in = [&](std::initializer_list<const char *> extensions) {
        return std::ranges::any_of(extensions, [&](const char *candidate) { return extension == candidate; });
    };
    if (in({".png", ".jpg", ".jpeg", ".bmp", ".gif", ".tga", ".webp"}))
        return Type::Image;
    if (in({".wav", ".ogg", ".mp3", ".flac"}))
        return Type::Audio;
    if (in({".ttf", ".otf"}))
        return Type::Font;
    if (extension == ".level")
        return Type::Level;
    if (in({".txt", ".md", ".json", ".ini", ".csv", ".xml"}))
        return Type::Text;
    return Type::Other;
}

const char *AssetLibrary::typeName(Type type)
{
    switch (type) {
    case Type::Image: return "Image";
    case Type::Audio: return "Audio";
    case Type::Font: return "Font";
    case Type::Level: return "Level";
    case Type::Text: return "Text";
    case Type::Other: break;
    }
    return "Other";
}
