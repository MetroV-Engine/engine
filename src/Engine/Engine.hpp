#pragma once

#include <iostream>
#include "Engine/Scenes/SceneManager.hpp"

/**
 * @brief Entry point of the engine: owns the scenes and runs the main loop.
 *
 * Usage:
 * @code
 * Engine engine;
 * engine.init();
 * engine.scenes().push(std::make_unique<MyScene>());
 * return engine.run();
 * @endcode
 */
class Engine {
    public:
        Engine() = default;
        ~Engine() = default;

        /**
         * @brief Initializes the engine. Must be called before run().
         * @return 0 on success, non-zero on failure.
         */
        int init();

        /**
         * @brief Runs the main loop: updates the scenes every frame until
         *        stop() is called or no scene is left.
         * @return 0 on a normal exit, non-zero if init() was not called.
         */
        int run();

        /**
         * @brief Asks the main loop to end after the current frame.
         * @return 0.
         */
        int stop();

        /** @brief The scene stack driven by run(). */
        SceneManager& scenes() { return _scenes; }

    private:
        SceneManager _scenes;
        bool _initialized = false;
        bool _running = false;
};
