#pragma once

/**
 * @file
 * @brief A character that runs and jumps: its tuning, its control mode and
 *        its state.
 */

#include <limits>

#include "Engine/Common/Math/Vector.hpp"

namespace Engine::Component {
    /**
     * @brief Turns a CharacterInput into the motion of a character's
     *        RigidBody (CharacterControllerSystem).
     *
     * The character is always a body with a mass, so its collisions stay
     * proportional: it pushes a light crate easily and a heavy one slowly,
     * and a moving platform carries it. Two modes decide who moves it:
     * - Controlled: the controller steers its velocity, precise like a
     *   kinematic body, but with a bounded acceleration, so impacts,
     *   explosions and heavy crates still count;
     * - Physics: the controller lets go, and the body is purely dynamic:
     *   a knockback, being thrown by an explosion, swinging on a rope.
     *   Control comes back after physicsTimeLeft, or when the game sets
     *   mode back to Controlled.
     *
     * The entity also needs a Transform, a RigidBody of mass > 0, a
     * Collider, ideally with Collider::Response::Stop so that it never
     * bounces, and a CharacterInput.
     *
     * Units are SI: m, m/s, m/s², s.
     */
    struct CharacterController {
        /** @brief Who moves the character. */
        enum class Mode {
            Controlled,    ///< The controller steers the velocity from the input.
            Physics,       ///< Physics alone: forces, impacts, friction.
        };

        // ------------------------------------------------------------ tuning

        /** @brief Running speed at full input, in m/s, relative to the ground it stands on. */
        float maxSpeed = 7.f;

        /** @brief On the ground, how fast the speed goes towards the input's, in m/s²: 0 to 7 m/s in 0.1 s. */
        float acceleration = 70.f;

        /** @brief On the ground with no input, how fast the character stops, in m/s². */
        float deceleration = 90.f;

        /**
         * @brief Friction from which the feet grip fully: the ground's
         *        combined muStatic (SurfaceContact::staticFriction), > 0.
         *
         * On grounds that grip at least this much (wood, stone, rubber:
         * about 0.5 with the default materials), acceleration and
         * deceleration apply whole. Below, the feet slip: both are scaled
         * by (mu / fullGripFriction)², so on ice (mu about 0.1) the
         * character gets a tenth of its control, speeds up slowly and slides
         * on when it stops running. Squared, so that the loss of control is
         * slight on a slightly slippery ground and strong on ice.
         */
        float fullGripFriction = 0.4f;

        /**
         * @brief Strongest push on what the character runs into, in N: a
         *        crate it pushes gets at most this force.
         *
         * acceleration is snappy for the character alone (70 kg at 70 m/s²
         * is 4900 N, superhuman); against a body in the way, the controller
         * pushes with this force instead, so the crate's mass and friction
         * decide: a 10 kg crate barely slows the character, a 100 kg one is
         * pushed slowly, one whose static friction exceeds this (above
         * about 200 kg on wood, mu 0.5) does not move. A strong person
         * pushes 500 to 800 N; a little more keeps the game responsive.
         */
        float pushStrength = 1000.f;

        /** @brief In the air, how fast the speed goes towards the input's, in m/s². */
        float airAcceleration = 40.f;

        /**
         * @brief In the air with no input, how fast the horizontal speed
         *        fades, in m/s². Low, so a jump keeps its momentum, a
         *        moving platform's included.
         */
        float airDeceleration = 5.f;

        /**
         * @brief Height of a full jump, in m: the feet rise exactly this
         *        much (Physic::speedToReachHeightForStep), whatever the
         *        gravity and the step.
         */
        float jumpHeight = 2.5f;

        /**
         * @brief Releasing jump while rising multiplies the upward speed by
         *        this, in [0, 1]: a tap makes a short hop. 1 disables it.
         */
        float jumpCutFactor = 0.5f;

        /**
         * @brief Gravity is multiplied by this while falling (>= 1): the
         *        fall is quicker than the rise, so the jump is not floaty.
         *        The rise, and so jumpHeight, is unaffected.
         */
        float fallGravityMultiplier = 1.5f;

        /**
         * @brief Fastest fall, in m/s, in both modes. Keep it under the
         *        thinnest platform's thickness times the step rate (60 m/s
         *        for 1 m at 60 Hz): faster, a step could skip through it.
         */
        float maxFallSpeed = 25.f;

        /**
         * @brief How far below the character the ground is looked for when
         *        it loses it without jumping, in m: running over the top of a
         *        slope or down a step, it is put back on the ground instead of
         *        flying off. Above maxSpeed * dt * tan(maxGroundAngle) (0.14 m
         *        at 7 m/s, 60 Hz, 50°); 0 disables it. A ledge deeper than
         *        this is a fall.
         */
        float groundSnapDistance = 0.3f;

        /** @brief After walking off a ledge, the character can still jump for this long, in s. */
        float coyoteTime = 0.1f;

        /** @brief A jump pressed this long before landing still happens on landing, in s. */
        float jumpBufferTime = 0.1f;

        /**
         * @brief Steepest surface that counts as ground, in degrees from
         *        flat. Steeper is a wall: no jump from it, no running on it.
         */
        float maxGroundAngle = 50.f;

        // -------------------------------------------------------------- mode

        /** @brief Controlled by default; see handToPhysics(). */
        Mode mode = Mode::Controlled;

        /**
         * @brief In Physics mode, seconds before control comes back. 0 or
         *        less: stays in Physics until the game sets Controlled.
         */
        float physicsTimeLeft = 0.f;

        /**
         * @brief Lets physics move the character for a while (a knockback),
         *        or until mode is set back (duration <= 0: a rope swing).
         *        Apply the impulse to the RigidBody's velocity as usual.
         */
        void handToPhysics(float duration) noexcept {
            mode = Mode::Physics;
            physicsTimeLeft = duration;
        }

        // ------------------------------------------- state, written by the system

        /** @brief What the character is doing, written by CharacterControllerSystem every step. */
        struct State {
            /** @brief Standing on ground (a surface flatter than maxGroundAngle). */
            bool grounded = false;

            /** @brief Touched the ground this step, after being in the air. */
            bool landed = false;

            /** @brief Jumped this step. */
            bool jumped = false;

            /** @brief -1 facing left, 1 facing right: the last direction moved towards. */
            int facing = 1;

            /** @brief Normal of the ground, pointing up out of it, when grounded. */
            Math::Vector2f groundNormal{0.f, 1.f};

            /** @brief Velocity of the ground stood on (a moving platform), or of the last one before jumping. */
            Math::Vector2f groundVelocity;

            /**
             * @brief Combined muStatic of the ground stood on, or of the last
             *        one: what the feet grip with (fullGripFriction).
             */
            float groundFriction = 1.f;

            /** @brief Seconds since the character last stood on ground. */
            float timeSinceGrounded = std::numeric_limits<float>::infinity();

            /** @brief Seconds since jump was last pressed and not yet used. */
            float timeSinceJumpPressed = std::numeric_limits<float>::infinity();

            /** @brief Rising from a jump that can still be cut short by releasing. */
            bool jumpCuttable = false;

            /** @brief Whether jump was held at the previous step, to detect presses. */
            bool jumpWasHeld = false;
        };

        State state;
    };
}
