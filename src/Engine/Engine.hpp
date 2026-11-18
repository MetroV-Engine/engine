#pragma once

#include <iostream>
#include <utility>

#include "Engine/Scenes/SceneManager.hpp"
#include "Engine/Services/Services.hpp"

/**
 * @brief Entry point of the engine: owns the services and the scenes, and
 *        drives them frame by frame.
 *
 * A game runs the whole loop with run(). A host that has its own loop (the
 * editor) calls step() once per frame instead.
 *
 * Usage:
 * @code
 * Engine engine;
 * engine.init();
 * engine.addService<AssetCache>();
 * engine.scenes().registry().add("menu", [] { return std::make_unique<MenuScene>(); });
 * engine.scenes().push("menu");
 * return engine.run();
 * @endcode
 */
class Engine {
    public:
        Engine();
        ~Engine() = default;

        Engine(const Engine&) = delete;
        Engine& operator=(const Engine&) = delete;

        /**
         * @brief Initializes the engine. Must be called before run() / step().
         * @return 0 on success, non-zero on failure.
         */
        int init();

        /**
         * @brief Runs one frame: updates the scenes with `dt` seconds.
         * @return 0 on success (also when no scene is left), 1 if init() was
         *         not called or `dt` is negative or NaN (nothing is updated).
         */
        int step(double dt);

        /**
         * @brief Runs the main loop: calls step() every frame until stop() is
         *        called or no scene is left, then exits the remaining scenes.
         * @return 0 on a normal exit, non-zero if init() was not called.
         */
        int run();

        /**
         * @brief Asks the main loop to end after the current frame.
         * @return 0.
         */
        int stop();

        /**
         * @brief Ends the engine: exits and destroys every scene, then every
         *        service. init() is needed again before run() / step().
         * @return 0.
         */
        int shutdown();

        /** @brief The scene stack driven by run() and step(). */
        SceneManager& scenes() { return _scenes; }

        /** @brief The services shared by every scene. */
        Services& services() { return _services; }

        /**
         * @brief Constructs (or replaces) a service and injects it into the
         *        scenes already on the stack. Scenes pushed later get it too.
         * @return Reference to the service.
         */
        template<typename T, typename... Params>
        T& addService(Params&&... params) {
            T& service = _services.set<T>(std::forward<Params>(params)...);
            _scenes.forEach([this](Scene& scene) { _services.injectInto(scene.registry()); });
            return service;
        }

    private:
        // Declared before the scenes: destroyed after them, so a scene can
        // still use a service in onExit().
        Services _services;
        SceneManager _scenes;
        bool _initialized = false;
        bool _running = false;
};
