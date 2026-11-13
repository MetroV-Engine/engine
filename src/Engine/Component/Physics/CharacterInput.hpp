#pragma once

/**
 * @file
 * @brief What the player (or an AI) asks a character to do this step.
 */

namespace Engine::Component {
    /**
     * @brief The intent of a character, written by the game every step and
     *        read by CharacterControllerSystem.
     *
     * The engine does not know the keyboard, gamepad or AI behind it: the
     * game turns its own input into these two values. An enemy uses the
     * same component, filled by its AI.
     */
    struct CharacterInput {
        /**
         * @brief Horizontal intent in [-1, 1]: -1 full speed left, 1 full
         *        speed right, 0 no input. A stick's partial tilt walks.
         */
        float move = 0.f;

        /**
         * @brief Whether jump is held. Write the button's state, not its
         *        press: the system detects the press itself, and releasing
         *        early makes a shorter jump.
         */
        bool jump = false;
    };
}
