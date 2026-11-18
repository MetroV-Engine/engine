#include "Engine/Engine.hpp"

#include <chrono>

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

    _running = true;
    while (_running && !_scenes.empty()) {
        const auto now = Clock::now();
        const double dt = std::chrono::duration<double>(now - previous).count();
        previous = now;

        step(dt);
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

int Engine::shutdown()
{
    _running = false;
    _scenes.clear();
    _services.clear();
    _initialized = false;
    return 0;
}
