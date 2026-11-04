/*
    Created on: 30/03/2026
    Filename: NotificationManager.hpp
    Description: ${DESCRIPTION}

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
#include <functional>
#include <utility>
#include <string>
#include <vector>

// Non-blocking feedback: small "toast" notifications stacked in the
// bottom-right corner. They disappear on their own after a delay (paused
// while hovered) or when clicked.
class NotificationManager
{
public:
    enum class Level
    {
        Info,
        Success,
        Warning,
        Error,
    };

    // Shows a notification. duration <= 0 uses the level's default
    // (errors and warnings stay longer). Returns its id.
    std::uint64_t notify(Level level, std::string title, std::string message = "", float duration = 0.0f);
    std::uint64_t info(std::string title, std::string message = "");
    std::uint64_t success(std::string title, std::string message = "");
    std::uint64_t warning(std::string title, std::string message = "");
    std::uint64_t error(std::string title, std::string message = "");

    // Returns false if no notification has this id (already gone).
    bool dismiss(std::uint64_t id);
    void clear() { _notifications.clear(); }

    // Draws the notifications and advances their timers. Call once per frame,
    // after the panels so they stay on top.
    void draw();

    std::size_t count() const { return _notifications.size(); }
    // Older notifications beyond this count are hidden until space frees up.
    void setMaxVisible(std::size_t maxVisible) { _maxVisible = maxVisible; }

    // Called for every new notification, e.g. to mirror it in a log or a status bar.
    using OnNotify = std::function<void(Level, const std::string &title, const std::string &message)>;
    void setOnNotify(OnNotify onNotify) { _onNotify = std::move(onNotify); }

private:
    struct Notification
    {
        std::uint64_t id;
        Level level;
        std::string title;
        std::string message;
        float duration;
        float remaining;
        float age = 0.0f;
        bool hovered = false;
    };

    std::vector<Notification> _notifications;
    std::uint64_t _nextId = 1;
    std::size_t _maxVisible = 5;
    OnNotify _onNotify;
};
