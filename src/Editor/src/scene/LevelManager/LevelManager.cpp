/*
    Created on: 10/11/2026
    Filename: LevelManager.cpp
    Description: The levels of the project: files opened into the scene and saved from it

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "LevelManager.hpp"
#include "scene/LevelFile/LevelFile.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <system_error>

namespace {

constexpr std::size_t MaxNameLength = 64;

} // namespace

LevelManager::LevelManager(HierarchyModel &hierarchy, PropertyModel &properties, const std::filesystem::path &folder)
    : _hierarchy(hierarchy), _properties(properties), _folder(std::filesystem::absolute(folder))
{
}

std::vector<std::string> LevelManager::list() const
{
    std::vector<std::string> names;
    std::error_code error;
    for (auto it = std::filesystem::directory_iterator(_folder, error); !error && it != std::filesystem::directory_iterator();
         it.increment(error)) {
        const std::filesystem::path &path = it->path();
        if (it->is_regular_file() && path.extension() == Extension && isValidName(path.stem().string()))
            names.push_back(path.stem().string());
    }
    std::ranges::sort(names);
    return names;
}

bool LevelManager::hasUnsavedChanges() const
{
    if (_current.empty())
        return !_hierarchy.roots().empty();
    return LevelFile::write(_hierarchy, _properties) != _savedText;
}

void LevelManager::create(const std::string &name)
{
    checkNew(name);
    for (HierarchyModel::ObjectId root : _hierarchy.roots())
        _hierarchy.destroy(root);
    write(name);
}

std::vector<std::string> LevelManager::open(const std::string &name)
{
    checkExists(name);
    std::ifstream file(pathOf(name), std::ios::binary);
    if (!file)
        throw std::runtime_error("Cannot read " + pathOf(name).string());
    const std::string text{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};

    std::vector<std::string> problems;
    try {
        problems = LevelFile::read(text, _hierarchy, _properties);
    } catch (const std::runtime_error &e) {
        throw std::runtime_error("\"" + name + "\" is not a valid level, " + e.what());
    }
    _current = name;
    // As the scene holds it: what it refused is not an unsaved change.
    _savedText = LevelFile::write(_hierarchy, _properties);
    return problems;
}

void LevelManager::save()
{
    if (_current.empty())
        throw std::runtime_error("No level is open: save the scene as a new level");
    write(_current);
}

void LevelManager::saveAs(const std::string &name)
{
    checkNew(name);
    write(name);
}

void LevelManager::rename(const std::string &from, const std::string &to)
{
    checkExists(from);
    checkNew(to);
    std::error_code error;
    std::filesystem::rename(pathOf(from), pathOf(to), error);
    if (error)
        throw std::runtime_error("Cannot rename \"" + from + "\": " + error.message());
    if (_current == from)
        _current = to;
}

void LevelManager::remove(const std::string &name)
{
    checkExists(name);
    std::error_code error;
    if (!std::filesystem::remove(pathOf(name), error))
        throw std::runtime_error("Cannot delete \"" + name + "\"" + (error ? ": " + error.message() : std::string()));
    if (_current == name) {
        _current.clear();
        _savedText.clear();
    }
}

bool LevelManager::isValidName(const std::string &name)
{
    return !name.empty() && name.size() <= MaxNameLength
        && std::ranges::all_of(name, [](unsigned char c) { return std::isalnum(c) || c == '-' || c == '_'; });
}

std::filesystem::path LevelManager::pathOf(const std::string &name) const
{
    return _folder / (name + Extension);
}

void LevelManager::checkNew(const std::string &name) const
{
    if (!isValidName(name))
        throw std::runtime_error("\"" + name + "\" is not a valid level name: use letters, digits, - and _");
    if (std::filesystem::exists(pathOf(name)))
        throw std::runtime_error("A level named \"" + name + "\" already exists");
}

void LevelManager::checkExists(const std::string &name) const
{
    if (!isValidName(name) || !std::filesystem::is_regular_file(pathOf(name)))
        throw std::runtime_error("No level named \"" + name + "\"");
}

void LevelManager::write(const std::string &name)
{
    std::error_code ignored;
    std::filesystem::create_directories(_folder, ignored);
    const std::string text = LevelFile::write(_hierarchy, _properties);
    std::ofstream file(pathOf(name), std::ios::binary | std::ios::trunc);
    file << text;
    file.close();
    if (!file)
        throw std::runtime_error("Cannot write " + pathOf(name).string());
    _current = name;
    _savedText = text;
}
