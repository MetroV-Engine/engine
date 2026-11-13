#pragma once

/**
 * @file
 * @brief Runs and jumps the characters from their input.
 */

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <string>

#include "ECS/ECS.hpp"
#include "Engine/Common/Math/Scalar.hpp"
#include "Engine/Common/Physic/Kinematics.hpp"
#include "Engine/Component/Physics/CharacterController.hpp"
#include "Engine/Component/Physics/CharacterInput.hpp"
#include "Engine/Component/Physics/Collider.hpp"
#include "Engine/Component/Physics/RigidBody.hpp"
#include "Engine/Component/Physics/SurfaceContact.hpp"
#include "Engine/System/Physics/Collision/Contacts.hpp"
#include "Engine/System/Physics/Gravity/GravitySettings.hpp"

namespace Engine::System {
    /**
     * @brief Moves every character (RigidBody + CharacterController +
     *        CharacterInput) as its input asks, in Controlled mode.
     *
     * Each step, for each character:
     * 1. Ground: it stands on ground when its SurfaceContact (set by
     *    CollisionResponseSystem) is flatter than maxGroundAngle. landed,
     *    the coyote and jump-buffer timers follow. Where the ground changes
     *    under it, the velocity leaving the new ground is removed: running
     *    up a slope onto flat ground does not launch it like a ramp. (On
     *    the same ground, a launch the game gives, a bounce pad, is kept.) Having just lost the ground without
     *    jumping (over the top of a slope, down a step), the character is
     *    swept down by groundSnapDistance and put back on any ground found.
     * 2. Jump, when pressed (or pressed less than jumpBufferTime ago) on
     *    the ground (or left less than coyoteTime ago): the upward velocity
     *    is set, relative to the ground, so that the feet rise exactly
     *    jumpHeight. Releasing jump while rising cuts the upward speed.
     * 3. Run: the velocity along the ground goes towards the input's, by
     *    at most acceleration * dt this step. Bounded, so the character
     *    never has an infinite force: an explosion still throws it. The
     *    target is relative to the ground, so a moving platform carries it.
     *    Running into a dynamic body (a contact of the last step, facing
     *    the way it runs), the force is at most pushStrength: the body's
     *    mass and friction decide how fast it goes.
     * 4. Fall: extra gravity while falling (fallGravityMultiplier), and the
     *    fall speed is capped at maxFallSpeed (in both modes).
     *
     * Running and the cap are forces added to RigidBody::force, computed
     * on the velocity the step's other forces will give: the controller
     * reaches its target exactly, whatever else pushes (a wind, a field),
     * within its acceleration. Jumps change the velocity at once.
     *
     * Grip: a running character's feet do not slide, the ground's static
     * friction is what pushes it forward. So in Controlled mode the
     * controller is its grip: the coefficients of its SurfaceContact are
     * set to 0 for the step (CollisionResponseSystem writes them again
     * every step), and FrictionSystem leaves it alone. Otherwise friction
     * would slow a runner more on rough ground than on ice, the opposite
     * of real life. In Physics mode the ground's friction applies, and a
     * thrown character slides to a stop.
     *
     * Slippery grounds: the grip is only as good as the ground. Below
     * CharacterController::fullGripFriction (the ground's combined
     * muStatic, kept in State::groundFriction), acceleration and
     * deceleration on the ground are scaled by (mu / fullGripFriction)²:
     * on ice the character is no longer steered at will, it speeds up
     * slowly and slides on after it stops running.
     *
     * Characters never rotate: the system sets RigidBody::fixedRotation, so
     * an off-center hit pushes the character without tipping it over.
     *
     * In Physics mode only the fall cap applies, and physicsTimeLeft runs
     * down; the body is left to forces, impacts and friction.
     *
     * Order: after the force systems, before FrictionSystem (which then
     * sees the grip) and IntegrationSystem (its forces are computed for the
     * same dt):
     * Gravity, other forces, CharacterController, Friction, Integration,
     * Collision, CollisionResponse.
     */
    class CharacterControllerSystem : public ECS::ISystem {
        public:
            void update(ECS::Registry& world, double dt) override {
                const float step = static_cast<float>(dt);
                if (step <= 0.f) {
                    return;
                }
                const Math::Vector2f gravity = world.hasResource<GravitySettings>()
                    ? world.getResource<GravitySettings>().gravity
                    : GravitySettings{}.gravity;
                const Math::Vector2f up = gravity.lengthSquared() > Math::Epsilon<float>
                    ? -gravity.normalized()
                    : Math::Vector2f::unitY();

                for (auto [entity, body, controller, input] :
                     world.view<Component::RigidBody, Component::CharacterController, Component::CharacterInput>()) {
                    if (body.mass <= Math::Epsilon<float>) {
                        continue;  // a kinematic body has no mass to move with forces
                    }
                    // A character stays upright: impacts and friction never turn it.
                    body.fixedRotation = true;
                    body.angularVelocity = 0.f;
                    Component::SurfaceContact* surface = world.hasComponents<Component::SurfaceContact>(entity)
                        ? &world.getComponent<Component::SurfaceContact>(entity)
                        : nullptr;
                    Character character{body, controller, input, gravity, up, step};
                    character.update(
                        surface,
                        [&world, entity](const Math::Vector2f& direction) { return runsInto(world, entity, direction); },
                        [&world, entity](const Math::Vector2f& delta, float flattest) {
                            return groundBelow(world, entity, delta, flattest);
                        });
                }
            }

            const std::string name() const override { return "CharacterControllerSystem"; }

        private:
            /** @brief Ground found below a character that lost it: how far to move, and the ground's normal and velocity. */
            struct Snap {
                Math::Vector2f move;
                Math::Vector2f normal;
                Math::Vector2f surfaceVelocity;
            };

            /**
             * @brief The first ground (normal within the flattest cosine) the
             *        character's collider meets moving by delta, against every
             *        other collider it can collide with (Component::canCollide);
             *        none without a Transform and a Collider.
             */
            static std::optional<Snap> groundBelow(
                ECS::Registry& world, ECS::Entity character, const Math::Vector2f& delta, float flattest) {
                if (!world.hasComponents<Component::Transform, Component::Collider>(character)) {
                    return std::nullopt;
                }
                const Component::Transform& transform = world.getComponent<Component::Transform>(character);
                const Component::Collider& collider = world.getComponent<Component::Collider>(character);
                const Math::Vector2f up = -delta.normalized();
                std::optional<Snap> first;
                float firstFraction = 2.f;
                for (auto [other, otherTransform, otherCollider] : world.view<Component::Transform, Component::Collider>()) {
                    if (other == character || !Component::canCollide(collider, otherCollider)) {
                        continue;
                    }
                    const std::optional<Collision::Hit> hit =
                        Component::sweep(collider, transform, delta, otherCollider, otherTransform);
                    if (!hit || hit->fraction >= firstFraction || hit->normal.dot(up) < flattest - Math::Epsilon<float>) {
                        continue;
                    }
                    firstFraction = hit->fraction;
                    const Math::Vector2f velocity = world.hasComponents<Component::RigidBody>(other)
                        ? world.getComponent<Component::RigidBody>(other).velocity
                        : Math::Vector2f{};
                    first = Snap{delta * hit->fraction, hit->normal, velocity};
                }
                if (first) {
                    world.getComponent<Component::Transform>(character).position += first->move;
                }
                return first;
            }

            /**
             * @brief Whether the character touched, at the last step, a
             *        dynamic body lying in the given direction (within 60
             *        degrees): what it pushes by running that way.
             */
            static bool runsInto(const ECS::Registry& world, ECS::Entity character, const Math::Vector2f& direction) {
                if (!world.hasResource<Contacts>()) {
                    return false;
                }
                constexpr float WithinSixtyDegrees = 0.5f;
                for (const CollisionPair& pair : world.getResource<Contacts>().pairs) {
                    if (pair.first != character && pair.second != character) {
                        continue;
                    }
                    const ECS::Entity other = pair.first == character ? pair.second : pair.first;
                    // The contact normal goes from first to second: towards the other body.
                    const Math::Vector2f towardsOther = pair.first == character ? pair.contact.normal : -pair.contact.normal;
                    if (towardsOther.dot(direction) >= WithinSixtyDegrees && world.hasComponents<Component::RigidBody>(other)
                        && world.getComponent<Component::RigidBody>(other).mass > Math::Epsilon<float>) {
                        return true;
                    }
                }
                return false;
            }

            /** @brief One character during one step. */
            struct Character {
                Component::RigidBody& body;
                Component::CharacterController& controller;
                const Component::CharacterInput& input;
                const Math::Vector2f& gravity;
                const Math::Vector2f& up;
                float step;

                using Mode = Component::CharacterController::Mode;

                template <typename RunsInto, typename GroundBelow>
                void update(Component::SurfaceContact* surface, const RunsInto& runsInto, const GroundBelow& groundBelow) {
                    Component::CharacterController::State& state = controller.state;
                    const bool wasGrounded = state.grounded;
                    const Math::Vector2f lastNormal = state.groundNormal;
                    updateGround(surface);
                    if (controller.mode == Mode::Controlled) {
                        const bool snapped = wasGrounded && !state.grounded && snapToGround(groundBelow);
                        // Only where the ground changes under it (over the top of a slope): on the same
                        // ground, a launch the game gives (a bounce pad) is kept.
                        if (snapped || (state.grounded && state.groundNormal != lastNormal)) {
                            stickToGround();
                        }
                    }
                    readJumpButton();
                    updateMode();
                    state.jumped = false;
                    if (controller.mode == Mode::Controlled) {
                        if (surface) {
                            // The controller is the grip: no friction from the ground this step.
                            surface->staticFriction = 0.f;
                            surface->kineticFriction = 0.f;
                        }
                        jump();
                        cutJump();
                        run(runsInto);
                        fallFaster();
                    } else {
                        state.jumpCuttable = false;
                    }
                    capFallSpeed();
                }

                void updateGround(const Component::SurfaceContact* surface) {
                    Component::CharacterController::State& state = controller.state;
                    const float flattest = std::cos(Math::toRadians(controller.maxGroundAngle));
                    const bool wasGrounded = state.grounded;
                    // A little tolerance, so a surface at exactly maxGroundAngle counts.
                    state.grounded = surface && surface->normal.dot(up) >= flattest - Math::Epsilon<float>;
                    state.landed = state.grounded && !wasGrounded;
                    if (state.grounded) {
                        state.groundNormal = surface->normal;
                        state.groundVelocity = surface->surfaceVelocity;
                        state.groundFriction = surface->staticFriction;
                        state.timeSinceGrounded = 0.f;
                    } else {
                        state.timeSinceGrounded += step;
                    }
                }

                /** @brief Puts the character back on ground just below, if any, after it lost the ground without jumping. */
                template <typename GroundBelow>
                bool snapToGround(const GroundBelow& groundBelow) {
                    Component::CharacterController::State& state = controller.state;
                    // Leaving the last ground along its normal (thrown up by it): a launch, not a ledge.
                    // Running up a slope moves along it, not away from it.
                    const float leaving = (body.velocity - state.groundVelocity).dot(state.groundNormal);
                    if (controller.groundSnapDistance <= 0.f || leaving > Math::Epsilon<float>) {
                        return false;
                    }
                    const float flattest = std::cos(Math::toRadians(controller.maxGroundAngle));
                    if (const auto snap = groundBelow(-up * controller.groundSnapDistance, flattest)) {
                        state.grounded = true;
                        state.groundNormal = snap->normal;
                        state.groundVelocity = snap->surfaceVelocity;
                        state.timeSinceGrounded = 0.f;
                        return true;
                    }
                    return false;
                }

                /** @brief On the ground, removes the velocity that would take the character off it (relative to the ground). */
                void stickToGround() {
                    const Component::CharacterController::State& state = controller.state;
                    if (!state.grounded) {
                        return;
                    }
                    const float leaving = (body.velocity - state.groundVelocity).dot(state.groundNormal);
                    if (leaving > 0.f) {
                        body.velocity -= state.groundNormal * leaving;
                    }
                }

                void readJumpButton() {
                    Component::CharacterController::State& state = controller.state;
                    state.timeSinceJumpPressed += step;
                    if (input.jump && !state.jumpWasHeld) {
                        state.timeSinceJumpPressed = 0.f;
                    }
                    state.jumpWasHeld = input.jump;
                }

                void updateMode() {
                    if (controller.mode != Mode::Physics || controller.physicsTimeLeft <= 0.f) {
                        return;  // controlled, or physics until the game says otherwise
                    }
                    controller.physicsTimeLeft -= step;
                    if (controller.physicsTimeLeft <= 0.f) {
                        controller.mode = Mode::Controlled;
                        controller.physicsTimeLeft = 0.f;
                    }
                }

                [[nodiscard]] float gravityMagnitude() const { return gravity.length() * body.gravityScale; }

                void jump() {
                    Component::CharacterController::State& state = controller.state;
                    const bool canJump = state.timeSinceGrounded <= controller.coyoteTime;
                    const bool wantsJump = state.timeSinceJumpPressed <= controller.jumpBufferTime;
                    if (!canJump || !wantsJump) {
                        return;
                    }
                    const float speed = Physic::speedToReachHeightForStep(controller.jumpHeight, gravityMagnitude(), step);
                    // Set, not added: a coyote jump while already falling rises as high.
                    const float upward = body.velocity.dot(up);
                    body.velocity += up * (state.groundVelocity.dot(up) + speed - upward);

                    constexpr float Never = std::numeric_limits<float>::infinity();
                    state.jumped = true;
                    state.jumpCuttable = true;
                    state.grounded = false;
                    state.timeSinceGrounded = Never;     // no second jump from the same ground
                    state.timeSinceJumpPressed = Never;  // the press is used
                }

                void cutJump() {
                    Component::CharacterController::State& state = controller.state;
                    if (!state.jumpCuttable) {
                        return;
                    }
                    const float upward = body.velocity.dot(up);
                    if (upward <= 0.f) {
                        state.jumpCuttable = false;  // past the top: nothing left to cut
                        return;
                    }
                    if (!input.jump) {
                        body.velocity += up * (upward * controller.jumpCutFactor - upward);
                        state.jumpCuttable = false;
                    }
                }

                /** @brief Velocity at the end of the step from the forces already added. */
                [[nodiscard]] Math::Vector2f predictedVelocity() const {
                    return body.velocity + body.force * (step / body.mass);
                }

                template <typename RunsInto>
                void run(const RunsInto& runsInto) {
                    Component::CharacterController::State& state = controller.state;
                    const float move = std::clamp(input.move, -1.f, 1.f);
                    if (std::fabs(move) > Math::Epsilon<float>) {
                        state.facing = move > 0.f ? 1 : -1;
                    }
                    // Along the ground, pointing right; in the air, the horizontal.
                    const Math::Vector2f right = -up.perpendicular();
                    Math::Vector2f axis = state.grounded ? -state.groundNormal.perpendicular() : right;
                    if (axis.dot(right) < 0.f) {
                        axis = -axis;
                    }
                    const float groundSpeed = state.grounded ? state.groundVelocity.dot(axis) : 0.f;
                    const float target = groundSpeed + move * controller.maxSpeed;
                    const float predicted = predictedVelocity().dot(axis);
                    const float change = target - predicted;
                    // Accelerating when the change goes the way the input asks;
                    // otherwise braking: no input, or faster than the input's speed.
                    const bool pushing = std::fabs(move) > Math::Epsilon<float> && change * move > 0.f;
                    float rate = state.grounded
                        ? (pushing ? controller.acceleration : controller.deceleration) * grip()
                        : (pushing ? controller.airAcceleration : controller.airDeceleration);
                    if (pushing && runsInto(axis * move)) {
                        rate = std::min(rate, controller.pushStrength / body.mass);
                    }
                    const float allowed = rate * step;
                    const float applied = std::clamp(change, -allowed, allowed);
                    body.force += axis * (body.mass * applied / step);
                }

                /**
                 * @brief Share of the ground control the feet get, in [0, 1]:
                 *        1 from fullGripFriction up, (mu / fullGripFriction)²
                 *        below (the feet slip on ice).
                 */
                [[nodiscard]] float grip() const {
                    const float full = controller.fullGripFriction;
                    const float friction = controller.state.groundFriction;
                    // A friction is never negative: a fullGripFriction of 0 or less always grips fully.
                    if (friction >= full) {
                        return 1.f;
                    }
                    const float share = std::max(0.f, friction) / full;
                    return share * share;
                }

                void fallFaster() {
                    if (controller.state.grounded || body.velocity.dot(up) >= 0.f) {
                        return;
                    }
                    body.force += gravity * (body.mass * body.gravityScale * (controller.fallGravityMultiplier - 1.f));
                }

                void capFallSpeed() {
                    const float upward = predictedVelocity().dot(up);
                    if (upward < -controller.maxFallSpeed) {
                        body.force += up * (body.mass * (-controller.maxFallSpeed - upward) / step);
                    }
                }
            };
    };
}
