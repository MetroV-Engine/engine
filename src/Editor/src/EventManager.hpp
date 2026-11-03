/*
    Created on: 30/03/2026
    Filename: EventManager.hpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#pragma once

#include <SDL3/SDL.h>

#include <cstdint>
#include <functional>
#include <unordered_map>
#include <vector>

class EventManager
{
public:
    using Callback = std::function<void(const SDL_Event &)>;

    // Which events are written to the log (category "Events", level Debug).
    enum class Logging
    {
        Off,
        // Everything except the continuous motion events (mouse, pen, touch),
        // which would flood the log at every frame.
        NoMotion,
        All,
    };

    // Called for every event of the given type (SDL_EventType).
    void subscribe(std::uint32_t type, Callback callback);
    // Called for every event, before type-specific callbacks.
    void subscribeAll(Callback callback);

    void pollEvents();
    // Blocks until an event arrives or timeoutMs elapses, then dispatches
    // everything queued. Used when there is nothing to draw.
    void waitEvents(int timeoutMs);

    void setLogging(Logging logging) { _logging = logging; }
    Logging getLogging() const { return _logging; }

private:
    std::vector<Callback> _globalCallbacks;
    std::unordered_map<std::uint32_t, std::vector<Callback>> _callbacks;
    Logging _logging = Logging::Off;

    void dispatch(const SDL_Event &event) const;
    void logEvent(const SDL_Event &event) const;
};
