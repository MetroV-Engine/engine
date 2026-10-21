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

#include <iostream>
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

        // logEvent(event); // Uncomment this line to log all events to the console
        dispatch(event);
    }
}

void EventManager::logEvent(const SDL_Event &event) const
{
    char description[256];
    SDL_GetEventDescription(&event, description, sizeof(description));
    std::cout << "[Event] " << description;
    if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP)
        std::cout << " key=\"" << SDL_GetKeyName(event.key.key) << "\"";
    std::cout << std::endl;
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
