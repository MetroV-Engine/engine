#include "Engine/Engine.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <thread>

Engine::Engine()
{
    _scenes.setInitializer([this](Scene& scene) { _services.injectInto(scene.registry()); });
}

int Engine::init()
{
    _initialized = true;
    return 0;
}

int Engine::step(double dt)
{
    if (!_initialized || !(dt >= 0.0))
        return 1;
    _scenes.update(dt);
    return 0;
}

int Engine::run()
{
    if (!_initialized)
        return 1;

    using Clock = std::chrono::steady_clock;
    auto previous = Clock::now();
    auto nextFrame = previous;

    _running = true;
    while (_running && !_scenes.empty()) {
        const auto now = Clock::now();
        double dt = std::chrono::duration<double>(now - previous).count();
        previous = now;
        if (_maxFrameTime > 0.0)
            dt = std::min(dt, _maxFrameTime);

        step(dt);

        // Frame rate cap: sleep until the next frame is due. A late frame
        // starts the schedule again from now instead of being caught up.
        if (_targetFrameRate > 0.0 && _running && !_scenes.empty()) {
            const auto period = std::chrono::duration_cast<Clock::duration>(
                std::chrono::duration<double>(1.0 / _targetFrameRate));
            nextFrame += period;
            const auto end = Clock::now();
            if (nextFrame < end)
                nextFrame = end;
            std::this_thread::sleep_until(nextFrame);
        }
    }
    _running = false;
    _scenes.clear();
    return 0;
}

bool Engine::setTargetFrameRate(double fps)
{
    if (!std::isfinite(fps) || fps < 0.0)
        return false;
    _targetFrameRate = fps;
    return true;
}

bool Engine::setMaxFrameTime(double seconds)
{
    if (!std::isfinite(seconds) || seconds < 0.0)
        return false;
    _maxFrameTime = seconds;
    return true;
}

int Engine::stop()
{
    _running = false;
    return 0;
}

int Engine::shutdown()
{
    _running = false;
    _scenes.clear();
    _services.clear();
    _initialized = false;
    return 0;
}
