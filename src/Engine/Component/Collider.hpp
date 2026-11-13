#pragma once

/**
 * @file
 * @brief The collision shape of an entity, whatever its form, and its material.
 */

#include <optional>
#include <type_traits>
#include <variant>

#include "Engine/Common/Collision/Collision.hpp"
#include "Engine/Common/Math/Vector.hpp"
#include "Transform.hpp"

namespace Engine::Component {
    /**
     * @brief What an entity collides with: one shape, placed relative to its
     *        Transform, and the material of its surface.
     *
     * One component for every shape, so the collision systems need a single
     * view: the shape is a std::variant, and worldShape() turns it into the
     * geometry of Engine/Common/Collision, where the entity stands.
     *
     * - With a RigidBody of mass > 0: a dynamic body, moved by collisions.
     * - With a RigidBody of mass 0: a kinematic body (a moving platform),
     *   which pushes others but is never pushed.
     * - Without a RigidBody: static scenery (a wall, the ground).
     *
     * The shape never rotates: boxes stay axis-aligned.
     */
    struct Collider {
        /** @brief Axis-aligned box centered on the collider. */
        struct Box {
            /** @brief Half the width and half the height, in m, > 0. */
            Math::Vector2f halfSize{0.5f, 0.5f};

            [[nodiscard]] constexpr bool operator==(const Box& other) const noexcept = default;
        };

        /** @brief Disc centered on the collider. */
        struct Circle {
            /** @brief Radius, in m, > 0. */
            float radius = 0.5f;

            [[nodiscard]] constexpr bool operator==(const Circle& other) const noexcept = default;
        };

        /** @brief The forms a collider can take, in its own space. Add a form here, then in worldShape(). */
        using Shape = std::variant<Box, Circle>;

        /** @brief The same forms placed in the world: what Collision::collide() and friends take. */
        using WorldShape = std::variant<Collision::Rectangle, Collision::Circle>;

        /** @brief The form: a 1 m box by default. */
        Shape shape = Box{};

        /** @brief Center of the shape relative to the Transform's position, in m. */
        Math::Vector2f offset;

        /** @brief What an impact does along the contact normal (CollisionResponseSystem). */
        enum class Response {
            /**
             * The default: the bodies bounce off each other with the
             * restitution of their materials, and share the impulse by mass
             * (a light crate flies off a heavy one, momentum is conserved).
             */
            Bounce,
            /**
             * Impacts involving this collider never bounce: the velocity
             * along the contact normal stops, the velocity along the
             * surface is kept (a character landing keeps running, one
             * hitting a wall keeps falling). Against another body, both
             * leave at the same speed along the normal, still shared by
             * mass. Put it on the character: on the ground, it would stop
             * everything landing there, bouncy balls included.
             */
            Stop,
        };

        /** @brief Bounce by default; Stop for a character that must never bounce. */
        Response response = Response::Bounce;

        /**
         * @brief Coefficient of restitution e, in [0, 1]: the speed after
         *        an impact over the speed before, along the normal. 0 stops
         *        dead, 1 bounces back at full speed. Two colliders bounce
         *        with the larger of their two (Physic::combinedRestitution),
         *        and never when one of them is Response::Stop.
         *
         * The default 0.3 is a wooden crate on a floor. About 0.8 for a
         * rubber ball, 0.05 for a sandbag.
         */
        float restitution = 0.3f;

        /**
         * @brief Static friction coefficient muStatic, >= 0 (SurfaceContact::staticFriction):
         *        about 0.1 on ice, 0.5 for wood on wood, 1 for rubber on concrete.
         */
        float staticFriction = 0.5f;

        /**
         * @brief Kinetic friction coefficient muKinetic, >= 0, usually below
         *        muStatic (SurfaceContact::kineticFriction): about 0.03 on
         *        ice, 0.3 for wood on wood, 0.8 for rubber on concrete.
         */
        float kineticFriction = 0.3f;

        /** @brief Center of the shape in the world, in m. */
        [[nodiscard]] constexpr Math::Vector2f center(const Transform& transform) const noexcept {
            return transform.position + offset;
        }

        /** @brief The shape where the entity stands, as collision geometry. */
        [[nodiscard]] constexpr WorldShape worldShape(const Transform& transform) const noexcept {
            const Math::Vector2f worldCenter = center(transform);
            return std::visit(
                [&worldCenter](const auto& form) -> WorldShape {
                    using Form = std::decay_t<decltype(form)>;
                    if constexpr (std::is_same_v<Form, Box>) {
                        return Collision::Rectangle::fromCenter(worldCenter, form.halfSize);
                    } else {
                        return Collision::Circle{worldCenter, form.radius};
                    }
                },
                shape);
        }

        /** @brief The smallest axis-aligned box containing the shape, in the world: for a broad phase. */
        [[nodiscard]] constexpr Collision::Rectangle boundingBox(const Transform& transform) const noexcept {
            return std::visit(
                [](const auto& placed) -> Collision::Rectangle {
                    using Placed = std::decay_t<decltype(placed)>;
                    if constexpr (std::is_same_v<Placed, Collision::Rectangle>) {
                        return placed;
                    } else {
                        return placed.boundingBox();
                    }
                },
                worldShape(transform));
        }
    };

    /**
     * @brief Contact between two colliders, whatever their forms, or none
     *        when they do not overlap.
     *
     * Calls the Collision::collide() of the two forms: the normal goes from
     * a towards b, and moving b by normal * depth separates them.
     */
    [[nodiscard]] inline std::optional<Collision::Contact> collide(
        const Collider& a, const Transform& transformA, const Collider& b, const Transform& transformB) noexcept {
        return std::visit(
            [](const auto& first, const auto& second) { return Collision::collide(first, second); },
            a.worldShape(transformA), b.worldShape(transformB));
    }
}
