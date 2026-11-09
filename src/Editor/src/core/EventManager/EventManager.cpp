/*
    Created on: 30/03/2026
    Filename: EventManager.cpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "EventManager.hpp"

#include "core/Logger/Logger.hpp"

#include <string>
#include <utility>

void EventManager::subscribe(std::uint32_t type, Callback callback)
{
    _callbacks[type].push_back(std::move(callback));
}

void EventManager::subscribeAll(Callback callback)
{
    _globalCallbacks.push_back(std::move(callback));
}

void EventManager::pollEvents()
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        logEvent(event);
        dispatch(event);
    }
}

void EventManager::waitEvents(int timeoutMs)
{
    SDL_Event event;
    if (!SDL_WaitEventTimeout(&event, timeoutMs))
        return;
    logEvent(event);
    dispatch(event);
    pollEvents();
}

void EventManager::logEvent(const SDL_Event &event) const
{
    if (_logging == Logging::Off)
        return;
    const bool motion = event.type == SDL_EVENT_MOUSE_MOTION || event.type == SDL_EVENT_PEN_MOTION
        || event.type == SDL_EVENT_FINGER_MOTION || event.type == SDL_EVENT_PEN_AXIS;
    if (motion && _logging == Logging::NoMotion)
        return;

    char description[256];
    SDL_GetEventDescription(&event, description, sizeof(description));
    std::string message = description;
    if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP)
        message += std::string(" key=\"") + SDL_GetKeyName(event.key.key) + "\"";
    Logger::instance().debug("Events", message);
}

void EventManager::dispatch(const SDL_Event &event) const
{
    for (const auto &callback : _globalCallbacks)
        callback(event);

    const auto it = _callbacks.find(event.type);
    if (it == _callbacks.end())
        return;
    for (const auto &callback : it->second)
        callback(event);
}
