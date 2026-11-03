/*
    Created on: 30/03/2026
    Filename: PlayController.cpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "PlayController.hpp"
#include "log/Logger.hpp"

#include <algorithm>
#include <exception>
#include <string>
#include <utility>

const char *PlayController::stateName(PlayState state)
{
    switch (state) {
    case PlayState::Playing:
        return "Playing";
    case PlayState::Paused:
        return "Paused";
    case PlayState::Stopped:
    default:
        return "Stopped";
    }
}

void PlayController::setState(PlayState state)
{
    if (state == _state)
        return;
    _state = state;
    if (_onStateChanged)
        _onStateChanged(state);
}

void PlayController::play()
{
    if (_state == PlayState::Playing)
        return;
    if (_state == PlayState::Stopped) {
        try {
            _simulation.begin();
        } catch (const std::exception &e) {
            Logger::instance().error("Game", std::string("Cannot start the game: ") + e.what());
            return;
        }
        _playTime = 0.0;
        _frames = 0;
    }
    setState(PlayState::Playing);
}

void PlayController::pause()
{
    if (_state == PlayState::Playing)
        setState(PlayState::Paused);
}

void PlayController::stop()
{
    if (_state == PlayState::Stopped)
        return;
    try {
        _simulation.end();
    } catch (const std::exception &e) {
        Logger::instance().error("Game", std::string("Error while stopping the game: ") + e.what());
    }
    setState(PlayState::Stopped);
}

void PlayController::step()
{
    if (_state == PlayState::Stopped) {
        play();
        if (_state == PlayState::Stopped)
            return; // begin() failed
    }
    pause();
    advance(StepDuration);
}

void PlayController::setTimeScale(double scale)
{
    _timeScale = std::clamp(scale, 0.05, 10.0);
}

void PlayController::update(double frameDuration)
{
    if (_state != PlayState::Playing)
        return;
    advance(std::min(frameDuration, MaxFrameDuration) * _timeScale);
}

bool PlayController::advance(double dt)
{
    try {
        _simulation.update(dt);
    } catch (const std::exception &e) {
        Logger::instance().error("Game", std::string("Game paused after an error: ") + e.what());
        setState(PlayState::Paused);
        return false;
    }
    _playTime += dt;
    _frames++;
    return true;
}
