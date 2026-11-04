/*
    Created on: 30/03/2026
    Filename: FrameTimer.cpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "FrameTimer.hpp"

#include <SDL3/SDL.h>

#include <algorithm>

void FrameTimer::tick()
{
    const std::uint64_t now = SDL_GetPerformanceCounter();
    _frameCount++;
    if (_lastCounter == 0) {
        _lastCounter = now;
        _deltaTime = 0.0;
        return;
    }
    _deltaTime = static_cast<double>(now - _lastCounter) / static_cast<double>(SDL_GetPerformanceFrequency());
    _lastCounter = now;

    _history[_historyOffset] = static_cast<float>(_deltaTime * 1000.0);
    _historyOffset = (_historyOffset + 1) % HistorySize;

    _windowTime += _deltaTime;
    _windowMaxTime = std::max(_windowMaxTime, _deltaTime);
    _windowFrames++;
    if (_windowTime >= RefreshInterval) {
        _fps = _windowFrames / _windowTime;
        _averageFrameTimeMs = _windowTime * 1000.0 / _windowFrames;
        _maxFrameTimeMs = _windowMaxTime * 1000.0;
        _windowTime = 0.0;
        _windowMaxTime = 0.0;
        _windowFrames = 0;
    }
}

void FrameTimer::reset()
{
    _lastCounter = 0;
    _deltaTime = 0.0;
    _windowTime = 0.0;
    _windowMaxTime = 0.0;
    _windowFrames = 0;
}
