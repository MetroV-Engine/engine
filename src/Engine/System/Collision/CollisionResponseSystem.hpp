#pragma once

/**
 * @file
 * @brief Separates the colliders that overlap and makes them bounce: the
 *        response half of collisions.
 */

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "CollisionSettings.hpp"
#include "Contacts.hpp"
#include "ECS/ECS.hpp"
#include "Engine/Common/Math/Scalar.hpp"
#include "Engine/Common/Physic/Forces/Dynamics.hpp"
#include "Engine/Common/Physic/Forces/Friction.hpp"
#include "Engine/Common/Physic/Impact.hpp"
#include "Engine/Component/Collider.hpp"
#include "Engine/Component/RigidBody.hpp"
#include "Engine/Component/SurfaceContact.hpp"
#include "Engine/Component/Transform.hpp"
#include "Engine/System/Gravity/GravitySettings.hpp"

namespace Engine::System {
    /**
     * @brief Resolves the pairs CollisionSystem found: the bodies stop going
     *        through each other, bounce, and get the SurfaceContact their
     *        friction needs.
     *
     * For each pair of Contacts, in order:
     * 1. The contact is computed again where the bodies are now: an earlier
     *    pair may have moved them (a crate on two tiles is pushed out of the
     *    first one, and then only touches the second, which must not push
     *    it a second time).
     * 2. Position: the overlap is removed (Physic::separation), shared by
     *    inverse mass. A static wall does not move, a body twice as heavy
     *    moves half as far, and their center of mass stays in place.
     * 3. Velocity: an impulse along the normal (Physic::bounceImpulse).
     *    Bodies moving apart are left alone; approaching ones bounce with
     *    e = combinedRestitution of their colliders, or e = 0 when one is
     *    Collider::Response::Stop or when they close slower than
     *    CollisionSettings::restingSpeed. Momentum is conserved and kinetic
     *    energy never grows. The velocity along the surface is left to
     *    friction.
     * 4. Static friction at the impact: when two dynamic bodies collide
     *    and one rests on its ground (its SurfaceContact, no sliding), the
     *    impulse that would push it along the ground is compared with what
     *    its static friction holds over the step, muStatic * N * dt
     *    (Coulomb's law over one step). Within it, the body does not move:
     *    for this impact it counts as fixed to its ground, and the other
     *    body takes the whole impulse and separation. A character pushing
     *    a crate too heavy for it is stopped, instead of making it creep;
     *    a lighter crate starts to slide, then kinetic friction acts.
     *    Steps 2 and 3 use the masses this decides.
     * 5. Friction of an impact: FrictionSystem slows bodies resting on a
     *    surface they already touched, from the forces pressing them on
     *    it. An impact (closing at restingSpeed or faster) or a new contact
     *    gets its friction here instead, as an impulse at the contact
     *    point bounded by Coulomb's law with the normal impulse: enough to
     *    stop the points sliding if mu static allows, else mu kinetic of
     *    it. A crate tipping onto a gentle slope stops there; a ball
     *    bouncing at an angle comes off spinning; a crate set on a slope
     *    does not start sliding on its first step. Not for
     *    Response::Stop, which keeps its velocity along the surface.
     * Then each dynamic body keeps one SurfaceContact: the surface it stands
     * on, the contact whose normal points most against gravity. Its
     * coefficients are the combinedFriction of the two colliders, and its
     * velocity the other body's (a moving platform carries what stands on
     * it). Bodies that touch nothing lose theirs.
     *
     * Masses come from the RigidBody: above Math::Epsilon a dynamic body;
     * 0 a kinematic one (a moving platform), which pushes and is never
     * pushed; no RigidBody, static scenery. Two of the last kinds never
     * respond to each other.
     *
     * Order: register it right after CollisionSystem, which runs after
     * IntegrationSystem. FrictionSystem reads the SurfaceContact in the next
     * step, after the forces.
     *
     * One pass over the pairs, with velocities updated as it goes: exact
     * for a body against scenery, approximate when several bodies push on
     * each other at once (a tall stack sinks slightly).
     */
    class CollisionResponseSystem : public ECS::ISystem {
        public:
            /** @brief Sets default CollisionSettings and Contacts resources when none were set. */
            void init(ECS::Registry& world) override {
                if (!world.hasResource<CollisionSettings>()) {
                    world.setResource<CollisionSettings>();
                }
                if (!world.hasResource<Contacts>()) {
                    world.setResource<Contacts>();
                }
            }

            void update(ECS::Registry& world, double dt) override {
                const Math::Vector2f gravity = world.hasResource<GravitySettings>()
                    ? world.getResource<GravitySettings>().gravity
                    : GravitySettings{}.gravity;
                const Step step{
                    world.getResource<CollisionSettings>().restingSpeed,
                    gravity.lengthSquared() > Math::Epsilon<float> ? -gravity.normalized() : Math::Vector2f::unitY(),
                    gravity.length(),
                    static_cast<float>(dt),
                };
                _surfaces.clear();
                for (const CollisionPair& pair : world.getResource<Contacts>().pairs) {
                    resolve(world, pair, step);
                }
                updateSurfaceContacts(world);
            }

            const std::string name() const override { return "CollisionResponseSystem"; }

        private:
            /** @brief One side of a pair, as the response sees it. */
            struct Side {
                Component::Transform& transform;
                const Component::Collider& collider;
                Component::RigidBody* body;  // nullptr: static scenery
                const Component::SurfaceContact* surface;  // the ground it stood on at the last step, if any
                float inverseInertia;  // 0: does not turn

                [[nodiscard]] float mass() const noexcept { return body ? body->mass : 0.f; }
                [[nodiscard]] Math::Vector2f velocity() const noexcept { return body ? body->velocity : Math::Vector2f{}; }
                [[nodiscard]] float angularVelocity() const noexcept { return body ? body->angularVelocity : 0.f; }
                [[nodiscard]] bool isDynamic() const noexcept { return body && body->mass > Math::Epsilon<float>; }

                /** @brief This side at the contact point, with these inverses (0 for what the impact cannot move). */
                [[nodiscard]] Physic::BodyPoint<float> at(const Math::Vector2f& point, float inverseMass, float inverseTurn) const noexcept {
                    return {velocity(), angularVelocity(), inverseMass, inverseTurn, point - transform.position};
                }

                /** @brief Applies an impulse at the contact point: v += J / m, ω += (arm x J) / I. */
                void push(const Physic::BodyPoint<float>& point, const Math::Vector2f& impulse) const noexcept {
                    if (!isDynamic()) {
                        return;
                    }
                    body->velocity += impulse * point.inverseMass;
                    body->angularVelocity += Physic::torque(point.arm, impulse) * point.inverseInertia;
                }
            };

            /** @brief A surface a dynamic body touches, with how much it faces up. */
            struct SurfaceCandidate {
                ECS::Entity entity;
                float facingUp = 0.f;
                Component::SurfaceContact contact;
            };

            /** @brief What every pair of the step needs. */
            struct Step {
                float restingSpeed;
                Math::Vector2f up;     // opposite of gravity, unit length; +Y when there is no gravity
                float gravity;         // |g|, in m/s²
                float dt;
            };

            /** @brief Sliding speed along its ground under which a body counts as resting on it, in m/s. */
            static constexpr float RestingSlide = 1e-3f;

            static Side side(ECS::Registry& world, ECS::Entity entity) {
                const Component::Collider& collider = world.getComponent<Component::Collider>(entity);
                Component::RigidBody* body = world.hasComponents<Component::RigidBody>(entity)
                    ? &world.getComponent<Component::RigidBody>(entity)
                    : nullptr;
                return {world.getComponent<Component::Transform>(entity), collider, body,
                        world.hasComponents<Component::SurfaceContact>(entity)
                            ? &world.getComponent<Component::SurfaceContact>(entity)
                            : nullptr,
                        body ? Component::inverseInertia(*body, &collider) : 0.f};
            }

            /**
             * @brief Whether static friction keeps a dynamic body on its ground
             *        against this impulse: it rests there, the impulse does not
             *        lift it, and the velocity it would give along the ground
             *        is at most muStatic * N / m * dt, with N / m = g * gravityScale
             *        * cos(slope).
             */
            static bool heldByStaticFriction(const Side& self, const Math::Vector2f& impulse, const Step& step) {
                if (!self.surface) {
                    return false;
                }
                const Math::Vector2f& normal = self.surface->normal;
                const Math::Vector2f sliding = self.body->velocity - self.surface->surfaceVelocity;
                if ((sliding - normal * sliding.dot(normal)).length() > RestingSlide) {
                    return false;  // already sliding: kinetic friction, applied by FrictionSystem
                }
                const Math::Vector2f change = Physic::velocityChangeFromImpulse(impulse, self.body->mass);
                if (change.dot(normal) > Math::Epsilon<float>) {
                    return false;  // lifted off its ground
                }
                const float along = (change - normal * change.dot(normal)).length();
                const float pressing = step.gravity * self.body->gravityScale * std::max(0.f, normal.dot(step.up));
                return along <= self.surface->staticFriction * pressing * step.dt;
            }

            void resolve(ECS::Registry& world, const CollisionPair& pair, const Step& step) {
                Side a = side(world, pair.first);
                Side b = side(world, pair.second);
                if (!a.isDynamic() && !b.isDynamic()) {
                    return;  // nothing here can be moved by an impact
                }
                const std::optional<Collision::Contact> contact =
                    Component::collide(a.collider, a.transform, b.collider, b.transform);
                if (!contact) {
                    return;  // already pushed apart by an earlier pair
                }
                const Math::Vector2f& normal = contact->normal;
                const Collision::Manifold manifold =
                    Component::contactPoints(a.collider, a.transform, b.collider, b.transform, *contact);
                const Math::Vector2f point = manifold.center();

                const bool stops = a.collider.response == Component::Collider::Response::Stop
                    || b.collider.response == Component::Collider::Response::Stop;
                const float restitution =
                    stops ? 0.f : Physic::combinedRestitution(a.collider.restitution, b.collider.restitution);
                float massA = a.mass();
                float massB = b.mass();
                float turnA = a.isDynamic() ? a.inverseInertia : 0.f;
                float turnB = b.isDynamic() ? b.inverseInertia : 0.f;
                Physic::BodyPoint<float> atA = a.at(point, Physic::inverseMass(massA), turnA);
                Physic::BodyPoint<float> atB = b.at(point, Physic::inverseMass(massB), turnB);
                const float closing = Physic::closingSpeed(atA.pointVelocity(), atB.pointVelocity(), normal);
                Math::Vector2f impulse = Physic::bounceImpulse(atA, atB, normal, restitution, step.restingSpeed);

                // Static friction at the impact. Against scenery or a platform there
                // is nothing to hold: the body has to move. Both held: neither is,
                // they share the impact.
                if (a.isDynamic() && b.isDynamic() && impulse != Math::Vector2f{}) {
                    const bool heldA = heldByStaticFriction(a, -impulse, step);
                    const bool heldB = heldByStaticFriction(b, impulse, step);
                    if (heldA != heldB) {
                        (heldA ? massA : massB) = 0.f;
                        (heldA ? turnA : turnB) = 0.f;
                        atA = a.at(point, Physic::inverseMass(massA), turnA);
                        atB = b.at(point, Physic::inverseMass(massB), turnB);
                        impulse = Physic::bounceImpulse(atA, atB, normal, restitution, step.restingSpeed);
                    }
                }

                // Friction during the impact: FrictionSystem only handles bodies resting on
                // a surface they already touched.
                const bool newContact = (a.isDynamic() && !a.surface) || (b.isDynamic() && !b.surface);
                const bool withFriction = !stops && (newContact || closing >= step.restingSpeed);
                solveVelocities(a, b, manifold, normal, atA, atB, restitution, step.restingSpeed, withFriction);
                solvePositions(a, b, manifold, normal, atA, atB);

                // The normal goes from a to b: out of a's surface, towards b.
                if (b.isDynamic()) {
                    _surfaces.push_back(candidate(pair.second, normal, point, manifold.count, a, b, step.up));
                }
                if (a.isDynamic()) {
                    _surfaces.push_back(candidate(pair.first, -normal, point, manifold.count, b, a, step.up));
                }
            }

            /** @brief A contact point being solved: its lever arms from each body's center. */
            struct Row {
                Math::Vector2f armA;
                Math::Vector2f armB;
            };

            /** @brief The rows of the manifold's points. */
            static std::array<Row, 2> rows(const Side& a, const Side& b, const Collision::Manifold& manifold) {
                std::array<Row, 2> result{};
                for (std::size_t i = 0; i < manifold.count; ++i) {
                    const Math::Vector2f& point = manifold.points[i].point;
                    result[i] = {point - a.transform.position, point - b.transform.position};
                }
                return result;
            }

            /**
             * @brief How much an impulse of 1 along the normal at point j moves
             *        point i along it: 1/mA + 1/mB + (rAi x n)(rAj x n)/IA +
             *        (rBi x n)(rBj x n)/IB. Symmetric; on the diagonal, the
             *        inverse of the point's effective mass.
             */
            static float coupling(const Row& i, const Row& j, const Math::Vector2f& normal,
                                  const Physic::BodyPoint<float>& atA, const Physic::BodyPoint<float>& atB) {
                return atA.inverseMass + atB.inverseMass
                    + atA.inverseInertia * i.armA.cross(normal) * j.armA.cross(normal)
                    + atB.inverseInertia * i.armB.cross(normal) * j.armB.cross(normal);
            }

            /**
             * @brief The impulses (>= 0) at one or two points such that each
             *        point gets at least `wanted` along the normal, and exactly
             *        that where it is pushed: a linear complementarity problem,
             *        solved exactly by trying both points pushing, then each
             *        alone, then neither (Box2D's block solver).
             *
             * @param k The coupling matrix (coupling()).
             * @param wanted What each point still needs along the normal.
             */
            static std::array<float, 2> complementary(const float k[2][2], const std::array<float, 2>& wanted, std::size_t count) {
                if (count == 1) {
                    return {k[0][0] > 0.f ? std::max(0.f, wanted[0] / k[0][0]) : 0.f, 0.f};
                }
                // Both pushing: K lambda = wanted.
                const float determinant = k[0][0] * k[1][1] - k[0][1] * k[1][0];
                if (std::fabs(determinant) > 1e-9f * (k[0][0] * k[1][1] + 1e-12f)) {
                    const float first = (wanted[0] * k[1][1] - wanted[1] * k[0][1]) / determinant;
                    const float second = (k[0][0] * wanted[1] - k[1][0] * wanted[0]) / determinant;
                    if (first >= 0.f && second >= 0.f) {
                        return {first, second};
                    }
                }
                // One alone: it gets what it needs, and the other must need no more than it gives.
                for (std::size_t only = 0; only < 2; ++only) {
                    const std::size_t other = 1 - only;
                    if (k[only][only] <= 0.f) {
                        continue;
                    }
                    const float alone = wanted[only] / k[only][only];
                    if (alone >= 0.f && k[other][only] * alone >= wanted[other] - 1e-6f * std::fabs(wanted[other])) {
                        std::array<float, 2> result{};
                        result[only] = alone;
                        return result;
                    }
                }
                return {0.f, 0.f};  // neither needs pushing
            }

            /** @brief Applies an impulse along the normal at a row's point: b gets it, a the opposite. */
            static void push(const Side& a, const Side& b, const Row& row, const Math::Vector2f& normal,
                             const Physic::BodyPoint<float>& atA, const Physic::BodyPoint<float>& atB, float amount) {
                const Math::Vector2f impulse = normal * amount;
                a.push(Physic::BodyPoint<float>{{}, 0.f, atA.inverseMass, atA.inverseInertia, row.armA}, -impulse);
                b.push(Physic::BodyPoint<float>{{}, 0.f, atB.inverseMass, atB.inverseInertia, row.armB}, impulse);
            }

            /** @brief How many rounds of friction then bounce: enough for friction and the two points to agree. */
            static constexpr int Iterations = 8;

            /**
             * @brief The bounce and, for an impact, the friction at the contact
             *        points, solved together (sequential impulses):
             * - normal: each point stops closing (or leaves at e times its
             *   closing speed), no point is pulled, and the points share the
             *   load as the bodies' masses and inertias decide (exact 2x2,
             *   complementary());
             * - friction: at each point, the impulse along the surface that
             *   stops the sliding, within Coulomb's bound: mu static times
             *   that point's normal impulse, mu kinetic once it slides.
             * They alternate a few rounds, each seeing what the other did: a
             * friction pulling at the bottom of a crate turns it, and the
             * normal impulses at its two corners turn it back, as the ground
             * pressing harder at the front does. On one point without
             * friction, Physic::bounceImpulse exactly.
             */
            static void solveVelocities(const Side& a, const Side& b, const Collision::Manifold& manifold,
                                        const Math::Vector2f& normal, const Physic::BodyPoint<float>& atA,
                                        const Physic::BodyPoint<float>& atB, float restitution, float restingSpeed,
                                        bool withFriction) {
                const std::array<Row, 2> pointRows = rows(a, b, manifold);
                const std::size_t count = manifold.count;
                const auto relativeVelocity = [&](const Row& row) {
                    return Physic::velocityAtPoint(b.velocity(), b.angularVelocity(), row.armB)
                        - Physic::velocityAtPoint(a.velocity(), a.angularVelocity(), row.armA);
                };
                float k[2][2] = {};
                std::array<float, 2> target{};
                for (std::size_t i = 0; i < count; ++i) {
                    for (std::size_t j = 0; j < count; ++j) {
                        k[i][j] = coupling(pointRows[i], pointRows[j], normal, atA, atB);
                    }
                    const float closing = -relativeVelocity(pointRows[i]).dot(normal);
                    target[i] = closing >= restingSpeed ? restitution * closing : 0.f;
                }

                // Along the surface: a fixed tangent, and how hard each point is to slide along it.
                const Math::Vector2f tangent = normal.perpendicular();
                std::array<float, 2> tangentMass{};
                for (std::size_t i = 0; i < count; ++i) {
                    const float resistance = coupling(pointRows[i], pointRows[i], tangent, atA, atB);
                    tangentMass[i] = resistance > 0.f ? 1.f / resistance : 0.f;
                }
                const float staticFriction = Physic::combinedFriction(a.collider.staticFriction, b.collider.staticFriction);
                const float kineticFriction = Physic::combinedFriction(a.collider.kineticFriction, b.collider.kineticFriction);

                std::array<float, 2> pressed{};   // normal impulse given so far at each point, >= 0
                std::array<float, 2> rubbed{};    // friction impulse given so far at each point, along the tangent
                const int rounds = withFriction ? Iterations : 1;
                for (int round = 0; round < rounds; ++round) {
                    // Normal: the total impulses that leave each point at its target, from what was given.
                    std::array<float, 2> wanted{};
                    for (std::size_t i = 0; i < count; ++i) {
                        wanted[i] = target[i] - relativeVelocity(pointRows[i]).dot(normal);
                        for (std::size_t j = 0; j < count; ++j) {
                            wanted[i] += k[i][j] * pressed[j];
                        }
                    }
                    const std::array<float, 2> total = complementary(k, wanted, count);
                    for (std::size_t i = 0; i < count; ++i) {
                        push(a, b, pointRows[i], normal, atA, atB, total[i] - pressed[i]);
                        pressed[i] = total[i];
                    }
                    if (!withFriction) {
                        break;
                    }
                    // Friction: stop each point sliding, within Coulomb's bound.
                    for (std::size_t i = 0; i < count; ++i) {
                        const float sliding = relativeVelocity(pointRows[i]).dot(tangent);
                        float rub = rubbed[i] - sliding * tangentMass[i];
                        const float grip = staticFriction * pressed[i];
                        if (std::fabs(rub) > grip) {
                            const float slip = kineticFriction * pressed[i];
                            rub = std::clamp(rub, -slip, slip);
                        }
                        push(a, b, pointRows[i], tangent, atA, atB, rub - rubbed[i]);
                        rubbed[i] = rub;
                    }
                }
            }

            /**
             * @brief Removes the overlap at the contact points, moving and
             *        turning the bodies by inverse mass and inertia (the same
             *        problem as the velocities, on positions, linearized). A
             *        box sunk at one corner is turned back level, not just
             *        lifted. On one point without rotation, Physic::separation
             *        exactly.
             */
            static void solvePositions(const Side& a, const Side& b, const Collision::Manifold& manifold,
                                       const Math::Vector2f& normal, const Physic::BodyPoint<float>& atA,
                                       const Physic::BodyPoint<float>& atB) {
                const std::array<Row, 2> pointRows = rows(a, b, manifold);
                float k[2][2] = {};
                std::array<float, 2> wanted{};
                for (std::size_t i = 0; i < manifold.count; ++i) {
                    for (std::size_t j = 0; j < manifold.count; ++j) {
                        k[i][j] = coupling(pointRows[i], pointRows[j], normal, atA, atB);
                    }
                    wanted[i] = manifold.points[i].depth;
                }
                const std::array<float, 2> amounts = complementary(k, wanted, manifold.count);
                for (std::size_t i = 0; i < manifold.count; ++i) {
                    const Row& row = pointRows[i];
                    if (a.isDynamic()) {
                        a.transform.position -= normal * (amounts[i] * atA.inverseMass);
                        a.transform.rotation -= row.armA.cross(normal) * amounts[i] * atA.inverseInertia;
                    }
                    if (b.isDynamic()) {
                        b.transform.position += normal * (amounts[i] * atB.inverseMass);
                        b.transform.rotation += row.armB.cross(normal) * amounts[i] * atB.inverseInertia;
                    }
                }
            }

            /** @brief The surface of `under` as seen by `body`, whose side of the contact `normal` points to. */
            static SurfaceCandidate candidate(ECS::Entity body, const Math::Vector2f& normal, const Math::Vector2f& point,
                                              std::size_t points, const Side& under, const Side& self, const Math::Vector2f& up) {
                Component::SurfaceContact contact;
                contact.normal = normal;
                contact.staticFriction = Physic::combinedFriction(self.collider.staticFriction, under.collider.staticFriction);
                contact.kineticFriction = Physic::combinedFriction(self.collider.kineticFriction, under.collider.kineticFriction);
                // The surface's own motion where it touches: a turning platform moves its rim.
                contact.surfaceVelocity = Physic::velocityAtPoint(under.velocity(), under.angularVelocity(), point - under.transform.position);
                // Touching at one point (a ball, a corner), friction acts there and turns the body.
                // Lying on a face (two points), the face keeps it from turning: through the center.
                contact.arm = points == 1 ? point - self.transform.position : Math::Vector2f{};
                return {body, normal.dot(up), contact};
            }

            /**
             * Structural changes (adding and removing SurfaceContact) are
             * queued: update() runs under the SystemManager's deferred scope,
             * so they apply at the end of the step, before FrictionSystem
             * reads them in the next one.
             */
            void updateSurfaceContacts(ECS::Registry& world) {
                // Per body, the candidate facing up the most comes first.
                std::sort(_surfaces.begin(), _surfaces.end(), [](const SurfaceCandidate& x, const SurfaceCandidate& y) {
                    if (x.entity != y.entity) {
                        return x.entity < y.entity;
                    }
                    return x.facingUp > y.facingUp;
                });
                const auto touches = [this](ECS::Entity entity) {
                    return std::binary_search(_surfaces.begin(), _surfaces.end(), entity,
                        [](const auto& x, const auto& y) { return entityOf(x) < entityOf(y); });
                };
                for (auto [entity, body, contact] : world.view<Component::RigidBody, Component::SurfaceContact>()) {
                    if (!touches(entity)) {
                        world.removeComponent<Component::SurfaceContact>(entity);
                    }
                }
                for (std::size_t i = 0; i < _surfaces.size(); ++i) {
                    if (i > 0 && _surfaces[i].entity == _surfaces[i - 1].entity) {
                        continue;  // not the best surface of this body
                    }
                    const SurfaceCandidate& best = _surfaces[i];
                    if (world.hasComponents<Component::SurfaceContact>(best.entity)) {
                        world.getComponent<Component::SurfaceContact>(best.entity) = best.contact;
                    } else {
                        world.addComponent(best.entity, Component::SurfaceContact{best.contact});
                    }
                }
            }

            static ECS::Entity entityOf(const SurfaceCandidate& surface) noexcept { return surface.entity; }
            static ECS::Entity entityOf(ECS::Entity entity) noexcept { return entity; }

            // Kept between steps: no allocation once it has grown.
            std::vector<SurfaceCandidate> _surfaces;
    };
}
