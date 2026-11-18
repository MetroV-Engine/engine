/*
    Created on: 10/11/2026
    Filename: LevelsPanel.cpp
    Description: Panel to create, open, save, rename and delete the levels of the project

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "LevelsPanel.hpp"
#include "core/Logger/Logger.hpp"
#include "ui/Button/Button.hpp"

#include <imgui.h>
#include <imgui_stdlib.h>

#include <stdexcept>
#include <utility>

namespace {

// Levels added or removed outside the editor show up after this delay.
constexpr double RefreshSeconds = 1.0;

} // namespace

LevelsPanel::LevelsPanel(LevelManager &levels, const PlayController &play, Selection &selection, NotificationManager &notifications,
    DialogManager &dialogs)
    : Panel("Levels"), _levels(levels), _play(play), _selection(selection), _notifications(notifications), _dialogs(dialogs)
{
}

void LevelsPanel::save()
{
    if (_levels.current().empty()) {
        setVisible(true);
        ImGui::SetWindowFocus(getName().c_str());
        startNaming(Naming::SaveAs, "");
        return;
    }
    if (apply("Cannot save the level", [this] { _levels.save(); }))
        _notifications.success("Level saved", _levels.current());
}

void LevelsPanel::drawContent()
{
    if (ImGui::GetTime() >= _nextRefresh) {
        _names = _levels.list();
        _nextRefresh = ImGui::GetTime() + RefreshSeconds;
    }
    const bool stopped = _play.isStopped();
    if (!stopped)
        ImGui::TextDisabled("Stop the game to change levels.");
    ImGui::BeginDisabled(!stopped);

    const std::string &current = _levels.current();
    if (current.empty())
        ImGui::TextDisabled("No level open: the scene is not saved.");
    else
        ImGui::Text("Level: %s", current.c_str());
    if (Button("Save").render())
        save();
    ImGui::SetItemTooltip("Save the scene in the open level");
    ImGui::SameLine();
    if (Button("Save As...", {}, Button::Variant::Secondary).render())
        startNaming(Naming::SaveAs, "");
    ImGui::SameLine();
    if (Button("New...", {}, Button::Variant::Secondary).render())
        startNaming(Naming::New, "");
    if (_naming != Naming::None)
        drawNameField();

    ImGui::Separator();
    drawList();
    ImGui::EndDisabled();
}

void LevelsPanel::startNaming(Naming naming, std::string name)
{
    _naming = naming;
    _name = std::move(name);
    _focusName = true;
}

void LevelsPanel::drawNameField()
{
    const char *verb = _naming == Naming::New ? "Create" : _naming == Naming::SaveAs ? "Save" : "Rename";
    const ImGuiStyle &style = ImGui::GetStyle();
    ImGui::SetNextItemWidth(-(ImGui::CalcTextSize(verb).x + style.FramePadding.x * 2.0f + style.ItemSpacing.x));
    if (_focusName) {
        ImGui::SetKeyboardFocusHere();
        _focusName = false;
    }
    const bool entered =
        ImGui::InputTextWithHint("##LevelName", "Name: letters, digits, - and _", &_name, ImGuiInputTextFlags_EnterReturnsTrue);
    // Escape cancels.
    if (ImGui::IsItemDeactivated() && ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        _naming = Naming::None;
        return;
    }
    ImGui::SameLine();
    if (Button(verb).render() || entered)
        confirmName();
}

void LevelsPanel::confirmName()
{
    const Naming naming = _naming;
    const std::string name = _name;
    _naming = Naming::None;
    bool done = true;
    switch (naming) {
    case Naming::New:
        discardChanges("New level", [this, name] { create(name); });
        break;
    case Naming::SaveAs:
        done = apply("Cannot save the level", [&] { _levels.saveAs(name); });
        if (done)
            _notifications.success("Level saved", name);
        break;
    case Naming::Rename:
        done = apply("Cannot rename the level", [&] { _levels.rename(_renamed, name); });
        if (done)
            _notifications.info("Level renamed", _renamed + " -> " + name);
        break;
    case Naming::None:
        break;
    }
    // A refused name can be corrected.
    if (!done)
        startNaming(naming, name);
}

void LevelsPanel::drawList()
{
    if (_names.empty()) {
        ImGui::TextDisabled("No level yet: Save As or New creates one.");
        return;
    }
    for (const std::string &name : _names) {
        ImGui::PushID(name.c_str());
        // Opening the open level again reverts its unsaved changes.
        if (ImGui::Selectable(name.c_str(), name == _levels.current(), ImGuiSelectableFlags_AllowDoubleClick)
            && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            discardChanges("Open level", [this, name] { open(name); });
        ImGui::SetItemTooltip("Double click to open, right click for more");
        if (ImGui::BeginPopupContextItem("##LevelMenu")) {
            if (ImGui::MenuItem("Open"))
                discardChanges("Open level", [this, name] { open(name); });
            if (ImGui::MenuItem("Rename")) {
                _renamed = name;
                startNaming(Naming::Rename, name);
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Delete"))
                requestDelete(name);
            ImGui::EndPopup();
        }
        ImGui::PopID();
    }
}

bool LevelsPanel::apply(const std::string &failure, const std::function<void()> &change)
{
    try {
        change();
    } catch (const std::runtime_error &e) {
        _notifications.warning(failure, e.what());
        return false;
    }
    _nextRefresh = 0.0;
    return true;
}

void LevelsPanel::discardChanges(const std::string &title, std::function<void()> then)
{
    if (!_levels.hasUnsavedChanges()) {
        then();
        return;
    }
    _dialogs.confirm({.title = title,
        .message = "The scene has unsaved changes. Discard them?",
        .confirmLabel = "Discard",
        .destructive = true,
        .onConfirm = std::move(then)});
}

void LevelsPanel::open(const std::string &name)
{
    std::vector<std::string> problems;
    if (!apply("Cannot open \"" + name + "\"", [&] { problems = _levels.open(name); }))
        return;
    _selection.clear();
    for (const std::string &problem : problems)
        Logger::instance().warning("Levels", name + ": " + problem);
    if (problems.empty())
        _notifications.success("Level opened", name);
    else
        _notifications.warning("Level opened with problems", "The console lists what the scene refused.");
}

void LevelsPanel::create(const std::string &name)
{
    if (!apply("Cannot create the level", [&] { _levels.create(name); }))
        return;
    _selection.clear();
    _notifications.success("Level created", name);
}

void LevelsPanel::requestDelete(const std::string &name)
{
    _dialogs.confirm({.title = "Delete level",
        .message = "Delete \"" + name + "\"? Its file is removed; the scene stays as it is.",
        .confirmLabel = "Delete",
        .destructive = true,
        .onConfirm = [this, name] {
            if (apply("Cannot delete \"" + name + "\"", [&] { _levels.remove(name); }))
                _notifications.info("Level deleted", name);
        }});
}
