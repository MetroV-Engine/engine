#pragma once

/**
 * @file
 * @brief The contacts found by CollisionSystem in the last step.
 */

#include <vector>

#include "ECS/ECS.hpp"
#include "Engine/Common/Physic/Collision/Contact.hpp"

namespace Engine::System {
    /** @brief Two colliders that overlap, and how they touch. */
    struct CollisionPair {
        /** @brief The first entity: contact.normal points away from it. */
        ECS::Entity first;

        /** @brief The second entity: moving it by contact.normal * contact.depth separates the two. */
        ECS::Entity second;

        /** @brief Normal from first towards second, depth and point, as Collision::collide() gives them. */
        Collision::Contact contact;
    };

    /**
     * @brief Resource: every pair of overlapping colliders, rebuilt by
     *        CollisionSystem at each step.
     *
     * What the collision response reads, and what gameplay code can read
     * too (a hitbox touching an enemy, a player entering a zone). Each pair
     * is listed once, with the entity of smaller bounding box min.x first;
     * pairs of two static colliders are never listed.
     */
    struct Contacts {
        std::vector<CollisionPair> pairs;
    };
}
