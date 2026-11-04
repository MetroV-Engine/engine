/*
    Created on: 30/03/2026
    Filename: DialogManager.hpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#pragma once

#include <cstdint>
#include <deque>
#include <functional>
#include <string>

// Blocking feedback: modal dialogs that ask for a confirmation or show a
// message before the user can continue. Dialogs are queued and shown one at
// a time; Enter confirms and Escape cancels.
class DialogManager
{
public:
    struct ConfirmOptions
    {
        std::string title;
        std::string message;
        std::string confirmLabel = "Confirm";
        std::string cancelLabel = "Cancel";
        // Destructive actions (delete, quit, discard) get a red confirm button.
        bool destructive = false;
        std::function<void()> onConfirm;
        std::function<void()> onCancel;
    };

    // Asks the user to confirm an action.
    void confirm(ConfirmOptions options);
    // Shows a message with a single OK button.
    void alert(std::string title, std::string message, std::function<void()> onClose = {});

    // True while a dialog is shown or waiting; editor shortcuts should be
    // ignored meanwhile.
    bool isOpen() const { return !_queue.empty(); }
    std::size_t pending() const { return _queue.size(); }

    // Draws the current dialog. Call once per frame, after the panels.
    void draw();

private:
    struct Dialog
    {
        std::uint64_t id;
        ConfirmOptions options;
        bool isAlert;
    };

    std::deque<Dialog> _queue;
    std::uint64_t _nextId = 1;
};
