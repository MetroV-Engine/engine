#pragma once

#include "ECS/Reflection/Reflection.hpp"

/**
 * @file
 * @brief The Sokoban's components: plain data, reflected so that the level
 *        file can describe them (and the editor show them).
 *
 * The level is made of entities with a Position and one of Player, Box,
 * Wall or Goal. A Box on a Goal is a placed box.
 */
namespace demo {
    /** @brief Cell of the grid: x to the right, y downwards. */
    struct Position {
        int x = 0;
        int y = 0;
    };

    /** @brief The entity the arrow keys move. It also has a MoveIntent. */
    struct Player {};

    /** @brief Pushed by the player; cannot be pulled, nor pushed into a wall or another box. */
    struct Box {};

    /** @brief Blocks the player and the boxes. */
    struct Wall {};

    /** @brief A cell where a box must end up. */
    struct Goal {};

    /** @brief The move asked for this frame (written by InputSystem, applied by MoveSystem). */
    struct MoveIntent {
        int dx = 0;
        int dy = 0;
    };
}

E_REFLECT(demo::Position)
    E_FIELD(x, int)
    E_FIELD(y, int)
E_END

E_REFLECT(demo::Player)
E_END

E_REFLECT(demo::Box)
E_END

E_REFLECT(demo::Wall)
E_END

E_REFLECT(demo::Goal)
E_END

E_REFLECT(demo::MoveIntent)
    E_FIELD(dx, int)
    E_FIELD(dy, int)
E_END
