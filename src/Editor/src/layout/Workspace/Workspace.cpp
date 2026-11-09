/*
    Created on: 30/03/2026
    Filename: Workspace.cpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "Workspace.hpp"
#include "core/Logger/Logger.hpp"

#include <imgui.h>
// The settings handler API (custom sections in imgui.ini) is internal.
#include <imgui_internal.h>

#include <cstdlib>
#include <utility>

namespace {

constexpr const char *SectionType = "Workspace";
// Seconds between two checks for changes (collecting is cheap but not free).
constexpr double CheckInterval = 0.5;

} // namespace

Workspace::Workspace(std::function<void(Values &)> collect, std::function<void(const Values &)> apply)
    : _collect(std::move(collect)), _apply(std::move(apply))
{
    ImGuiSettingsHandler handler;
    handler.TypeName = SectionType;
    handler.TypeHash = ImHashStr(SectionType);
    handler.UserData = this;
    handler.ClearAllFn = [](ImGuiContext *, ImGuiSettingsHandler *h) {
        Workspace &self = *static_cast<Workspace *>(h->UserData);
        self._read.clear();
        self._found = false;
    };
    handler.ReadOpenFn = [](ImGuiContext *, ImGuiSettingsHandler *h, const char *name) -> void * {
        Workspace &self = *static_cast<Workspace *>(h->UserData);
        if (std::string(name) != "Editor")
            return nullptr;
        self._found = true;
        return &self;
    };
    handler.ReadLineFn = [](ImGuiContext *, ImGuiSettingsHandler *, void *entry, const char *line) {
        Workspace &self = *static_cast<Workspace *>(entry);
        const std::string text = line;
        const std::size_t equal = text.find('=');
        if (equal != std::string::npos)
            self._read[text.substr(0, equal)] = text.substr(equal + 1);
    };
    handler.WriteAllFn = [](ImGuiContext *, ImGuiSettingsHandler *h, ImGuiTextBuffer *out) {
        std::string text;
        static_cast<const Workspace *>(h->UserData)->write(text);
        out->append(text.c_str());
    };
    ImGui::AddSettingsHandler(&handler);
}

Workspace::~Workspace()
{
    if (!ImGui::GetCurrentContext())
        return;
    save();
    // ImGui saves again when its context is destroyed; without this section
    // (removed below) it would drop the workspace from the file.
    ImGui::RemoveSettingsHandler(SectionType);
    ImGui::GetIO().IniFilename = nullptr;
}

bool Workspace::load()
{
    const char *path = ImGui::GetIO().IniFilename;
    if (path)
        ImGui::LoadIniSettingsFromDisk(path);
    _saved = _read;
    if (!_found)
        return false;
    _apply(_read);
    Logger::instance().info("Editor", std::string("Workspace restored from ") + path);
    return true;
}

void Workspace::save() const
{
    if (const char *path = ImGui::GetIO().IniFilename)
        ImGui::SaveIniSettingsToDisk(path);
}

void Workspace::update()
{
    const double now = ImGui::GetTime();
    if (now < _nextCheck)
        return;
    _nextCheck = now + CheckInterval;
    Values current;
    _collect(current);
    if (current == _saved)
        return;
    _saved = std::move(current);
    // ImGui writes the file a few seconds later (io.IniSavingRate).
    ImGui::MarkIniSettingsDirty();
}

void Workspace::write(std::string &out) const
{
    Values values;
    _collect(values);
    out += "[" + std::string(SectionType) + "][Editor]\n";
    for (const auto &[key, value] : values)
        out += key + "=" + value + "\n";
    out += "\n";
}

bool Workspace::getBool(const Values &values, const std::string &key, bool fallback)
{
    const auto it = values.find(key);
    return it == values.end() ? fallback : it->second == "1" || it->second == "true";
}

int Workspace::getInt(const Values &values, const std::string &key, int fallback)
{
    const auto it = values.find(key);
    if (it == values.end())
        return fallback;
    char *end = nullptr;
    const long value = std::strtol(it->second.c_str(), &end, 10);
    return end && *end == '\0' && !it->second.empty() ? static_cast<int>(value) : fallback;
}

std::string Workspace::getString(const Values &values, const std::string &key, const std::string &fallback)
{
    const auto it = values.find(key);
    return it == values.end() ? fallback : it->second;
}
