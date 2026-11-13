#pragma once

/**
 * @file
 * @brief Finds the colliders that overlap: the detection half of collisions.
 */

#include <algorithm>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "Contacts.hpp"
#include "ECS/ECS.hpp"
#include "Engine/Common/Physic/Collision/Overlap.hpp"
#include "Engine/Component/Physics/Collider.hpp"
#include "Engine/Component/Physics/RigidBody.hpp"
#include "Engine/Component/Transform.hpp"

namespace Engine::System {
    /**
     * @brief Lists every pair of overlapping colliders in the Contacts
     *        resource, with its normal and depth.
     *
     * Detection only: nothing moves here. A response system reads Contacts
     * to separate the bodies and make them bounce, and gameplay code can
     * read it too. Register it after IntegrationSystem, so it sees where
     * the bodies ended up.
     *
     * Every entity with a Transform and a Collider takes part. Two static
     * colliders (no RigidBody) never move, so their pair is skipped: a level
     * made of many touching tiles costs nothing. Neither are two colliders
     * whose layers refuse each other (Component::canCollide): they pass
     * through each other, and only cost the test of two bit masks.
     *
     * Broad phase, "sort and sweep": the bounding boxes are sorted by their
     * left edge, then each box is only tested against the boxes that start
     * before its right edge ends. For n colliders spread along X, that is
     * O(n log n) instead of the n² / 2 tests of every pair. Only the pairs
     * whose boxes overlap go to the exact test, Component::collide().
     *
     * Edges count: colliders that only touch are listed, with depth 0.
     */
    class CollisionSystem : public ECS::ISystem {
        public:
            /** @brief Sets an empty Contacts resource when none was set. */
            void init(ECS::Registry& world) override {
                if (!world.hasResource<Contacts>()) {
                    world.setResource<Contacts>();
                }
            }

            void update(ECS::Registry& world, double) override {
                std::vector<CollisionPair>& pairs = world.getResource<Contacts>().pairs;
                pairs.clear();
                gather(world);
                sweep(pairs);
            }

            const std::string name() const override { return "CollisionSystem"; }

        private:
            /** @brief A collider as the broad phase sees it. */
            struct Entry {
                ECS::Entity entity;
                const Component::Transform* transform = nullptr;
                const Component::Collider* collider = nullptr;
                Collision::Rectangle bounds;
                bool isStatic = false;
            };

            void gather(ECS::Registry& world) {
                // Kept between steps: no allocation once it has grown. The
                // pointers stay valid during the step: update() runs under
                // the SystemManager's deferred scope, so nothing is added or
                // removed meanwhile.
                _entries.clear();
                for (auto [entity, transform, collider] : world.view<Component::Transform, Component::Collider>()) {
                    _entries.push_back({entity, &transform, &collider, collider.boundingBox(transform),
                                        !world.hasComponents<Component::RigidBody>(entity)});
                }
                // Ties broken by entity, so the order of the pairs does not
                // depend on the sort implementation.
                std::sort(_entries.begin(), _entries.end(), [](const Entry& a, const Entry& b) {
                    if (a.bounds.min.x != b.bounds.min.x) {
                        return a.bounds.min.x < b.bounds.min.x;
                    }
                    return a.entity < b.entity;
                });
            }

            void sweep(std::vector<CollisionPair>& pairs) const {
                for (std::size_t i = 0; i < _entries.size(); ++i) {
                    const Entry& a = _entries[i];
                    // Sorted by min.x: once a box starts after a ends, so do all the next ones.
                    for (std::size_t j = i + 1; j < _entries.size() && _entries[j].bounds.min.x <= a.bounds.max.x; ++j) {
                        const Entry& b = _entries[j];
                        if ((a.isStatic && b.isStatic) || !Component::canCollide(*a.collider, *b.collider)
                            || !Collision::overlaps(a.bounds, b.bounds)) {
                            continue;
                        }
                        if (const std::optional<Collision::Contact> contact =
                                Component::collide(*a.collider, *a.transform, *b.collider, *b.transform)) {
                            pairs.push_back({a.entity, b.entity, *contact});
                        }
                    }
                }
            }

            std::vector<Entry> _entries;
    };
}
