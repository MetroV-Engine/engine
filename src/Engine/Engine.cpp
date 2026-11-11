#include "Engine/Engine.hpp"

#include <chrono>

int Engine::init()
{
    _initialized = true;
    return 0;
}

int Engine::run()
{
    if (!_initialized)
        return 1;

    using Clock = std::chrono::steady_clock;
    auto previous = Clock::now();

    _running = true;
    while (_running && !_scenes.empty()) {
        const auto now = Clock::now();
        const double dt = std::chrono::duration<double>(now - previous).count();
        previous = now;

        _scenes.update(dt);
    }
    _running = false;
    _scenes.clear();
    return 0;
}

int Engine::stop()
{
    _running = false;
    return 0;
}
