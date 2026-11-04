/*
    Created on: 30/03/2026
    Filename: FrameTimer.hpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

// Measures frame time and FPS. tick() is called once per frame; values are
// averaged over a short window so they stay readable on screen.
class FrameTimer
{
public:
    static constexpr std::size_t HistorySize = 120;

    // Marks the start of a new frame.
    void tick();
    // Forgets the previous frame (e.g. after the loop was paused), so the next
    // delta does not include the pause.
    void reset();

    // Duration of the last frame, in seconds.
    double getDeltaTime() const { return _deltaTime; }
    // Averages over the last refresh window (~0.5 s).
    double getFps() const { return _fps; }
    double getAverageFrameTimeMs() const { return _averageFrameTimeMs; }
    double getMaxFrameTimeMs() const { return _maxFrameTimeMs; }
    std::uint64_t getFrameCount() const { return _frameCount; }

    // Last frame times in ms, as a ring buffer starting at getHistoryOffset().
    const std::array<float, HistorySize> &getHistory() const { return _history; }
    std::size_t getHistoryOffset() const { return _historyOffset; }

private:
    static constexpr double RefreshInterval = 0.5;

    std::uint64_t _lastCounter = 0;
    double _deltaTime = 0.0;
    std::uint64_t _frameCount = 0;

    double _windowTime = 0.0;
    double _windowMaxTime = 0.0;
    int _windowFrames = 0;
    double _fps = 0.0;
    double _averageFrameTimeMs = 0.0;
    double _maxFrameTimeMs = 0.0;

    std::array<float, HistorySize> _history{};
    std::size_t _historyOffset = 0;
};
