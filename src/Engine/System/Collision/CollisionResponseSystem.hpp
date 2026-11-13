#pragma once

/**
 * @file
 * @brief Separates the colliders that overlap and makes them bounce: the
 *        response half of collisions.
 */

#include <algorithm>
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

            void update(ECS::Registry& world, double) override {
                const float restingSpeed = world.getResource<CollisionSettings>().restingSpeed;
                const Math::Vector2f up = upDirection(world);
                _surfaces.clear();
                for (const CollisionPair& pair : world.getResource<Contacts>().pairs) {
                    resolve(world, pair, restingSpeed, up);
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

                [[nodiscard]] float mass() const noexcept { return body ? body->mass : 0.f; }
                [[nodiscard]] Math::Vector2f velocity() const noexcept { return body ? body->velocity : Math::Vector2f{}; }
                [[nodiscard]] bool isDynamic() const noexcept { return body && body->mass > Math::Epsilon<float>; }
            };

            /** @brief A surface a dynamic body touches, with how much it faces up. */
            struct SurfaceCandidate {
                ECS::Entity entity;
                float facingUp = 0.f;
                Component::SurfaceContact contact;
            };

            static Side side(ECS::Registry& world, ECS::Entity entity) {
                return {world.getComponent<Component::Transform>(entity), world.getComponent<Component::Collider>(entity),
                        world.hasComponents<Component::RigidBody>(entity) ? &world.getComponent<Component::RigidBody>(entity)
                                                                          : nullptr};
            }

            /** @brief Opposite of gravity, unit length; +Y when there is no gravity. */
            static Math::Vector2f upDirection(const ECS::Registry& world) {
                const Math::Vector2f gravity = world.hasResource<GravitySettings>()
                    ? world.getResource<GravitySettings>().gravity
                    : GravitySettings{}.gravity;
                return gravity.lengthSquared() > Math::Epsilon<float> ? -gravity.normalized() : Math::Vector2f::unitY();
            }

            void resolve(ECS::Registry& world, const CollisionPair& pair, float restingSpeed, const Math::Vector2f& up) {
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

                const Physic::Separation<float> moves = Physic::separation(normal, contact->depth, a.mass(), b.mass());
                a.transform.position += moves.first;
                b.transform.position += moves.second;

                const bool stops = a.collider.response == Component::Collider::Response::Stop
                    || b.collider.response == Component::Collider::Response::Stop;
                const float restitution =
                    stops ? 0.f : Physic::combinedRestitution(a.collider.restitution, b.collider.restitution);
                const Math::Vector2f impulse = Physic::bounceImpulse(
                    a.velocity(), b.velocity(), normal, a.mass(), b.mass(), restitution, restingSpeed);
                if (a.isDynamic()) {
                    a.body->velocity -= Physic::velocityChangeFromImpulse(impulse, a.body->mass);
                }
                if (b.isDynamic()) {
                    b.body->velocity += Physic::velocityChangeFromImpulse(impulse, b.body->mass);
                }

                // The normal goes from a to b: out of a's surface, towards b.
                if (b.isDynamic()) {
                    _surfaces.push_back(candidate(pair.second, normal, a, b, up));
                }
                if (a.isDynamic()) {
                    _surfaces.push_back(candidate(pair.first, -normal, b, a, up));
                }
            }

            /** @brief The surface of `under` as seen by `body`, whose side of the contact `normal` points to. */
            static SurfaceCandidate candidate(
                ECS::Entity body, const Math::Vector2f& normal, const Side& under, const Side& self, const Math::Vector2f& up) {
                Component::SurfaceContact contact;
                contact.normal = normal;
                contact.staticFriction = Physic::combinedFriction(self.collider.staticFriction, under.collider.staticFriction);
                contact.kineticFriction = Physic::combinedFriction(self.collider.kineticFriction, under.collider.kineticFriction);
                contact.surfaceVelocity = under.velocity();
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
