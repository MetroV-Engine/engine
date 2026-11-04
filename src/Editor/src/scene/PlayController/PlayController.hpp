/*
    Created on: 30/03/2026
    Filename: PlayController.hpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#pragma once

#include "scene/Simulation.hpp"

#include <cstdint>
#include <functional>

enum class PlayState
{
    Stopped, // Editing: the simulation does not run
    Playing,
    Paused,
};

// Play / Pause / Stop / Step controls of the editor. It runs a Simulation from
// the editor's frame loop, with a time scale, and keeps the play time.
//
//   Stopped --play--> Playing <--play/pause--> Paused
//      ^                 |                       |
//      +------stop-------+---------stop----------+
//
// A failure of the simulation (exception) is logged and pauses the game,
// so a gameplay bug never closes the editor.
class PlayController
{
public:
    // Duration of one Step, and the longest frame given to the simulation
    // (a frame after a hitch is cut, so physics does not explode).
    static constexpr double StepDuration = 1.0 / 60.0;
    static constexpr double MaxFrameDuration = 0.1;

    explicit PlayController(Simulation &simulation) : _simulation(simulation) {}

    // Starts the game, or resumes it when paused.
    void play();
    void pause();
    // Ends the game and restores the edited state. No effect when stopped.
    void stop();
    // Advances one frame and pauses (starts the game paused if stopped).
    void step();

    // Call once per frame with the real frame duration.
    void update(double frameDuration);

    PlayState getState() const { return _state; }
    bool isStopped() const { return _state == PlayState::Stopped; }
    // Seconds of game time since Play (scaled, without the paused time).
    double getPlayTime() const { return _playTime; }
    std::uint64_t getFrameCount() const { return _frames; }
    double getTimeScale() const { return _timeScale; }
    // Clamped to [0.05, 10].
    void setTimeScale(double scale);

    void setOnStateChanged(std::function<void(PlayState)> callback) { _onStateChanged = std::move(callback); }

    static const char *stateName(PlayState state);

private:
    Simulation &_simulation;
    PlayState _state = PlayState::Stopped;
    double _playTime = 0.0;
    std::uint64_t _frames = 0;
    double _timeScale = 1.0;
    std::function<void(PlayState)> _onStateChanged;

    void setState(PlayState state);
    // Runs one simulation update; returns false (and pauses) if it failed.
    bool advance(double dt);
};
