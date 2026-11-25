#pragma once

#include "Engine/Engine.hpp"
#include "Engine/Game/GameCatalog.hpp"
#include "demo/Components.hpp"
#include "demo/Systems.hpp"
#include "demo/Terminal.hpp"

/**
 * @file
 * @brief The demo game: its components and systems (included here), and how
 *        it prepares the Engine.
 *
 * Registered with E_GAME: a generic program (the runtime) finds it in the
 * GameCatalog and calls install() without naming the demo. Compiled into the
 * program by Demo.cpp (the demo_game OBJECT library); a plugin will later
 * register the same way when its shared library is loaded.
 */
namespace demo {
    /** @brief Adds the demo's services to the engine: the terminal it draws on and reads keys from. */
    inline void install(Engine& engine) {
        engine.addService<Terminal>();
    }
}

E_GAME(demo, demo::install)
