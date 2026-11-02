/*
    Created on: 30/03/2026
    Filename: DialogManager.cpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "DialogManager.hpp"
#include "components/Button.hpp"

#include <imgui.h>

#include <utility>

namespace {

// Sizes in font heights, so dialogs follow the display scale.
constexpr float MessageWidthInFontSizes = 26.0f;
constexpr float ButtonWidthInFontSizes = 7.0f;

} // namespace

void DialogManager::confirm(ConfirmOptions options)
{
    _queue.push_back({_nextId++, std::move(options), false});
}

void DialogManager::alert(std::string title, std::string message, std::function<void()> onClose)
{
    ConfirmOptions options;
    options.title = std::move(title);
    options.message = std::move(message);
    options.confirmLabel = "OK";
    options.onConfirm = onClose;
    options.onCancel = std::move(onClose);
    _queue.push_back({_nextId++, std::move(options), true});
}

void DialogManager::draw()
{
    if (_queue.empty())
        return;

    const Dialog &dialog = _queue.front();
    const ConfirmOptions &options = dialog.options;
    // The id keeps two dialogs with the same title from sharing ImGui state.
    const std::string popupId = options.title + "##Dialog" + std::to_string(dialog.id);
    if (!ImGui::IsPopupOpen(popupId.c_str()))
        ImGui::OpenPopup(popupId.c_str());

    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->GetWorkCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));

    enum class Result
    {
        None,
        Confirmed,
        Cancelled,
    };
    Result result = Result::None;
    const ImGuiWindowFlags flags =
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove;
    if (ImGui::BeginPopupModal(popupId.c_str(), nullptr, flags)) {
        const float fontSize = ImGui::GetFontSize();
        ImGui::PushTextWrapPos(fontSize * MessageWidthInFontSizes);
        ImGui::TextUnformatted(options.message.c_str());
        ImGui::PopTextWrapPos();
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        const ImVec2 buttonSize(fontSize * ButtonWidthInFontSizes, 0.0f);
        const Button confirmButton(options.confirmLabel, {},
            options.destructive ? Button::Variant::Danger : Button::Variant::Primary, buttonSize);
        if (confirmButton.render() || ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter))
            result = Result::Confirmed;
        if (!dialog.isAlert) {
            ImGui::SameLine();
            const Button cancelButton(options.cancelLabel, {}, Button::Variant::Secondary, buttonSize);
            if (cancelButton.render())
                result = Result::Cancelled;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Escape))
            result = Result::Cancelled;

        if (result != Result::None)
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
    if (result == Result::None)
        return;

    // Removed before the callback runs, so a callback may queue another dialog.
    const std::function<void()> callback = result == Result::Confirmed ? options.onConfirm : options.onCancel;
    _queue.pop_front();
    if (callback)
        callback();
}
