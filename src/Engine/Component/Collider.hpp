#pragma once

/**
 * @file
 * @brief The collision shape of an entity, whatever its form, and its material.
 */

#include <optional>
#include <type_traits>
#include <variant>

#include "Engine/Common/Collision/Collision.hpp"
#include "Engine/Common/Math/Scalar.hpp"
#include "Engine/Common/Math/Vector.hpp"
#include "Engine/Common/Physic/Rotation.hpp"
#include "RigidBody.hpp"
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
     * The shape turns with the Transform's rotation, around the Transform's
     * position (the body's center of mass): offset turns too. A turned box
     * becomes a Polygon in the world; unturned, it stays an exact
     * axis-aligned Rectangle. A slope is a Polygon:
     * Collider::Polygon::slope() builds the right triangle of one.
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

        /**
         * @brief Convex polygon: a slope, a ramp, a cut corner. Its vertices
         *        are relative to the collider's center, counter-clockwise,
         *        strictly convex (Shapes::Polygon's invariant, which
         *        points.isConvex() checks).
         */
        struct Polygon {
            Shapes::Polygon points;

            /**
             * @brief The right triangle filling the box of this half size
             *        below its diagonal: a slope rising to the right (or to
             *        the left), at atan(height / width). Equal half sizes
             *        give 45°, a width twice the height 26.6°.
             */
            [[nodiscard]] static constexpr Polygon slope(const Math::Vector2f& halfSize, bool risesToTheRight = true) noexcept {
                const Math::Vector2f bottomLeft = -halfSize;
                const Math::Vector2f bottomRight{halfSize.x, -halfSize.y};
                if (risesToTheRight) {
                    return {{bottomLeft, bottomRight, halfSize}};
                }
                return {{bottomLeft, bottomRight, {-halfSize.x, halfSize.y}}};
            }

            [[nodiscard]] constexpr bool operator==(const Polygon& other) const noexcept = default;
        };

        /** @brief The forms a collider can take, in its own space. Add a form here, then in worldShape(). */
        using Shape = std::variant<Box, Circle, Polygon>;

        /** @brief The same forms placed in the world: what Collision::collide() and friends take. */
        using WorldShape = std::variant<Collision::Rectangle, Collision::Circle, Collision::Polygon>;

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

        /** @brief Center of the shape in the world, in m: the offset turned with the Transform. */
        [[nodiscard]] constexpr Math::Vector2f center(const Transform& transform) const noexcept {
            return transform.position + (transform.rotation == 0.f ? offset : offset.rotated(transform.rotation));
        }

        /**
         * @brief Moment of inertia about the Transform's position (the center
         *        of mass) for this mass spread uniformly over the shape, in
         *        kg·m²: the shape's own (Physic::boxInertia, discInertia,
         *        polygonInertia) moved by the offset (parallel axis).
         */
        [[nodiscard]] float momentOfInertia(float mass) const noexcept {
            return std::visit(
                [this, mass](const auto& form) -> float {
                    using Form = std::decay_t<decltype(form)>;
                    if constexpr (std::is_same_v<Form, Box>) {
                        return Physic::parallelAxis(Physic::boxInertia(mass, form.halfSize), mass, offset);
                    } else if constexpr (std::is_same_v<Form, Circle>) {
                        return Physic::parallelAxis(Physic::discInertia(mass, form.radius), mass, offset);
                    } else {
                        return Physic::parallelAxis(
                            Physic::polygonInertia(mass, form.points), mass, offset + form.points.centroid());
                    }
                },
                shape);
        }

        /**
         * @brief A form placed in the world: its center at the Transform's
         *        position plus offset (turned with it), turned by its
         *        rotation. What worldShape() does with the collider's own
         *        shape; Buoyant places its shape the same way.
         */
        [[nodiscard]] static constexpr WorldShape place(
            const Shape& form, const Math::Vector2f& offset, const Transform& transform) noexcept {
            const float turn = transform.rotation;
            const Math::Vector2f worldCenter = transform.position + (turn == 0.f ? offset : offset.rotated(turn));
            return std::visit(
                [&worldCenter, turn](const auto& kind) -> WorldShape {
                    using Form = std::decay_t<decltype(kind)>;
                    if constexpr (std::is_same_v<Form, Box>) {
                        if (turn == 0.f) {
                            return Collision::Rectangle::fromCenter(worldCenter, kind.halfSize);
                        }
                        return Collision::Polygon::fromRectangle(Collision::Rectangle::fromCenter({}, kind.halfSize))
                            .rotated(turn)
                            .translated(worldCenter);
                    } else if constexpr (std::is_same_v<Form, Circle>) {
                        return Collision::Circle{worldCenter, kind.radius};
                    } else {
                        return (turn == 0.f ? kind.points : kind.points.rotated(turn)).translated(worldCenter);
                    }
                },
                form);
        }

        /** @brief The shape where the entity stands, as collision geometry. */
        [[nodiscard]] constexpr WorldShape worldShape(const Transform& transform) const noexcept {
            return place(shape, offset, transform);
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

    /**
     * @brief Where two colliders that collide press on each other, given
     *        their contact (collide(a, ..., b, ...)): one or two points
     *        (Collision::contactPoints).
     */
    [[nodiscard]] inline Collision::Manifold contactPoints(const Collider& a, const Transform& transformA, const Collider& b,
                                                           const Transform& transformB, const Collision::Contact& contact) noexcept {
        return std::visit(
            [&contact](const auto& first, const auto& second) { return Collision::contactPoints(first, second, contact); },
            a.worldShape(transformA), b.worldShape(transformB));
    }

    /**
     * @brief First contact of a collider moving by delta with a still one,
     *        whatever their forms, or none.
     *
     * Calls the Collision::sweep() of the two forms: the moving collider
     * moved by delta * fraction touches the other, and the normal is the
     * still one's surface there, pointing back at the moving one. Colliders
     * that already overlap give no hit.
     */
    [[nodiscard]] inline std::optional<Collision::Hit> sweep(const Collider& moving, const Transform& transform,
        const Math::Vector2f& delta, const Collider& still, const Transform& stillTransform) noexcept {
        return std::visit(
            [&delta](const auto& first, const auto& second) { return Collision::sweep(first, delta, second); },
            moving.worldShape(transform), still.worldShape(stillTransform));
    }

    /**
     * @brief 1 / I of a body about its center of mass, or 0 when it does
     *        not rotate: fixedRotation, static or kinematic (mass at most
     *        Math::Epsilon: impacts do not turn it), or no Collider.
     */
    [[nodiscard]] inline float inverseInertia(const RigidBody& body, const Collider* collider) noexcept {
        if (!collider || body.fixedRotation || body.mass <= Math::Epsilon<float>) {
            return 0.f;
        }
        return Physic::inverseInertia(collider->momentOfInertia(body.mass));
    }
}
