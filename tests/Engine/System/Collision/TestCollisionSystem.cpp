#include <boost/ut.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <optional>
#include <random>
#include <tuple>
#include <vector>

#include "ECS/ECS.hpp"
#include "Engine/Component/Collider.hpp"
#include "Engine/System/Collision/CollisionSystem.hpp"
#include "Engine/System/Integration/IntegrationSystem.hpp"
#include "../../Support/Approx.hpp"

using namespace boost::ut;
using namespace Engine;
using Component::Collider;
using Component::RigidBody;
using Component::Transform;
using System::CollisionPair;
using System::CollisionSystem;
using System::Contacts;
using TestSupport::near;
using V = Math::Vector2f;

namespace {
constexpr double Step = 1.0 / 60.0;

Collider box(V halfSize, V offset = {}) {
    Collider collider;
    collider.shape = Collider::Box{halfSize};
    collider.offset = offset;
    return collider;
}

Collider circle(float radius, V offset = {}) {
    Collider collider;
    collider.shape = Collider::Circle{radius};
    collider.offset = offset;
    return collider;
}

/** @brief A static collider: no RigidBody. */
ECS::Entity spawnStatic(ECS::Registry& world, V position, const Collider& collider) {
    const ECS::Entity entity = world.spawnEntity();
    world.addComponent(entity, Transform{position});
    world.addComponent(entity, Collider{collider});
    return entity;
}

ECS::Entity spawnBody(ECS::Registry& world, V position, const Collider& collider, float mass = 1.f) {
    const ECS::Entity entity = spawnStatic(world, position, collider);
    world.addComponent(entity, RigidBody{mass});
    return entity;
}

const std::vector<CollisionPair>& detect(ECS::Registry& world, ECS::SystemManager& systems) {
    systems.update(world, Step);
    return world.getResource<Contacts>().pairs;
}

bool sameContact(const Collision::Contact& actual, const Collision::Contact& expected) {
    return actual.normal == expected.normal && actual.depth == expected.depth && actual.point == expected.point;
}

void test_detection() {
    "init sets an empty Contacts resource, and keeps one already set"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;
        systems.addSystem<CollisionSystem>(world);
        expect(world.hasResource<Contacts>());
        expect(world.getResource<Contacts>().pairs.empty());

        ECS::Registry other;
        other.setResource<Contacts>().pairs.push_back({});
        ECS::SystemManager otherSystems;
        otherSystems.addSystem<CollisionSystem>(other);
        expect(other.getResource<Contacts>().pairs.size() == 1_u);
        expect(otherSystems.getSystem<CollisionSystem>()->name() == "CollisionSystem");
    };

    "two overlapping colliders give one pair, with the contact of Component::collide"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;
        systems.addSystem<CollisionSystem>(world);
        const ECS::Entity ground = spawnStatic(world, V(0.f, -1.f), box(V(5.f, 1.f)));
        const ECS::Entity ball = spawnBody(world, V(1.f, 0.4f), circle(0.5f));

        const std::vector<CollisionPair>& pairs = detect(world, systems);

        expect(fatal(pairs.size() == 1_u));
        // The ground starts further left, so it comes first: the normal points up, to the ball.
        expect(pairs[0].first == ground);
        expect(pairs[0].second == ball);
        expect(near(pairs[0].contact.normal, V(0.f, 1.f), 1e-6f));
        expect(near(pairs[0].contact.depth, 0.1f, 1e-6f));
        const auto expected = Component::collide(world.getComponent<Collider>(ground), world.getComponent<Transform>(ground),
                                                 world.getComponent<Collider>(ball), world.getComponent<Transform>(ball));
        expect(fatal(expected.has_value()));
        expect(sameContact(pairs[0].contact, *expected));
    };

    "colliders apart give no pair, colliders that only touch give one of depth 0"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;
        systems.addSystem<CollisionSystem>(world);
        spawnBody(world, V(0.f, 0.f), box(V(0.5f, 0.5f)));
        spawnBody(world, V(1.5f, 0.f), box(V(0.5f, 0.5f)));     // 0.5 m to the right: apart
        spawnBody(world, V(0.f, 1.f), box(V(0.5f, 0.5f)));      // right on top: touching

        const std::vector<CollisionPair>& pairs = detect(world, systems);

        expect(fatal(pairs.size() == 1_u));
        expect(pairs[0].contact.depth == 0.f);
        expect(pairs[0].contact.normal == V(0.f, 1.f));
    };

    "colliders touching side by side are paired: one starts exactly where the other ends"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;
        systems.addSystem<CollisionSystem>(world);
        const ECS::Entity wall = spawnStatic(world, V(0.f, 0.f), box(V(0.5f, 2.f)));
        const ECS::Entity crate = spawnBody(world, V(1.f, 0.f), box(V(0.5f, 0.5f)));

        const std::vector<CollisionPair>& pairs = detect(world, systems);

        expect(fatal(pairs.size() == 1_u));
        expect(pairs[0].first == wall);
        expect(pairs[0].second == crate);
        expect(pairs[0].contact.depth == 0.f);
        expect(pairs[0].contact.normal == V(1.f, 0.f));
    };

    "boxes side by side along X but at different heights do not collide"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;
        systems.addSystem<CollisionSystem>(world);
        spawnBody(world, V(0.f, 0.f), box(V(1.f, 0.5f)));
        spawnBody(world, V(0.5f, 3.f), box(V(1.f, 0.5f)));
        spawnBody(world, V(-0.5f, 1.2f), circle(0.5f));

        expect(detect(world, systems).empty());
    };

    "the offset of a collider counts, not only its Transform"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;
        systems.addSystem<CollisionSystem>(world);
        spawnBody(world, V(0.f, 0.f), box(V(0.5f, 0.5f), V(3.f, 0.f)));
        spawnBody(world, V(3.5f, 0.f), box(V(0.5f, 0.5f)));

        expect(detect(world, systems).size() == 1_u);
    };
}

void test_who_takes_part() {
    "two static colliders are never paired, a body with each of them is"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;
        systems.addSystem<CollisionSystem>(world);
        // Two overlapping tiles and a box overlapping both.
        const ECS::Entity left = spawnStatic(world, V(0.f, 0.f), box(V(0.6f, 0.5f)));
        const ECS::Entity right = spawnStatic(world, V(1.f, 0.f), box(V(0.6f, 0.5f)));
        const ECS::Entity crate = spawnBody(world, V(0.5f, 0.8f), box(V(0.5f, 0.5f)));

        const std::vector<CollisionPair>& pairs = detect(world, systems);

        expect(fatal(pairs.size() == 2_u));
        for (const CollisionPair& pair : pairs) {
            expect(pair.first == crate || pair.second == crate);
            expect(pair.first != pair.second);
        }
        expect(pairs[0].first == left || pairs[0].second == left || pairs[1].first == left || pairs[1].second == left);
        expect(pairs[0].first == right || pairs[0].second == right || pairs[1].first == right || pairs[1].second == right);
    };

    "a kinematic body (mass 0) is paired with static colliders and other bodies"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;
        systems.addSystem<CollisionSystem>(world);
        spawnStatic(world, V(0.f, 0.f), box(V(1.f, 1.f)));
        spawnBody(world, V(1.5f, 0.f), box(V(1.f, 1.f)), 0.f);
        spawnBody(world, V(3.f, 0.f), box(V(1.f, 1.f)), 0.f);

        expect(detect(world, systems).size() == 2_u);
    };

    "an entity without a Transform or without a Collider is ignored"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;
        systems.addSystem<CollisionSystem>(world);
        spawnBody(world, V(0.f, 0.f), box(V(1.f, 1.f)));
        const ECS::Entity noTransform = world.spawnEntity();
        world.addComponent(noTransform, box(V(1.f, 1.f)));
        world.addComponent(noTransform, RigidBody{});
        const ECS::Entity noCollider = world.spawnEntity();
        world.addComponent(noCollider, Transform{V(0.f, 0.f)});
        world.addComponent(noCollider, RigidBody{});

        expect(detect(world, systems).empty());
    };

    "the pairs are rebuilt every step: bodies that moved apart are gone"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;
        systems.addSystem<CollisionSystem>(world);
        spawnBody(world, V(0.f, 0.f), circle(1.f));
        const ECS::Entity other = spawnBody(world, V(1.f, 0.f), circle(1.f));
        expect(detect(world, systems).size() == 1_u);

        world.getComponent<Transform>(other).position = V(5.f, 0.f);

        expect(detect(world, systems).empty());
    };

    "after IntegrationSystem, a body that fell into the ground is detected in the same step"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;
        systems.addSystem<Engine::System::IntegrationSystem>(world);
        systems.addSystem<CollisionSystem>(world);
        spawnStatic(world, V(0.f, -1.f), box(V(5.f, 1.f)));
        const ECS::Entity crate = spawnBody(world, V(0.f, 0.55f), box(V(0.5f, 0.5f)));
        world.getComponent<RigidBody>(crate).velocity = V(0.f, -6.f);    // 0.1 m per step

        const std::vector<CollisionPair>& pairs = detect(world, systems);

        expect(fatal(pairs.size() == 1_u));
        expect(near(pairs[0].contact.depth, 0.05f, 1e-5f));
    };
}

// The sort and sweep must find exactly the pairs of the brute-force test of
// every pair, in a random scene: same pairs, same order, same contacts.
void test_against_every_pair() {
    "random scenes: the same pairs as testing every pair of colliders"_test = [] {
        struct Expected {
            ECS::Entity first;
            ECS::Entity second;
            Collision::Contact contact;
        };

        for (unsigned seed = 1; seed <= 20; ++seed) {
            std::mt19937 random(seed);
            const auto uniform = [&random](float low, float high) {
                return std::uniform_real_distribution<float>(low, high)(random);
            };
            ECS::Registry world;
            ECS::SystemManager systems;
            systems.addSystem<CollisionSystem>(world);

            // A row of static tiles on whole meters (many equal min.x), and
            // random shapes above them: static, kinematic or dynamic.
            for (int tile = 0; tile < 40; ++tile) {
                spawnStatic(world, V(static_cast<float>(tile), 0.f), box(V(0.5f, 0.5f)));
            }
            for (int index = 0; index < 160; ++index) {
                const V position(std::round(uniform(0.f, 40.f) * 2.f) / 2.f, uniform(0.f, 6.f));
                const V offset = index % 4 == 0 ? V(uniform(-1.f, 1.f), uniform(-1.f, 1.f)) : V::zero();
                const Collider collider = index % 2 == 0 ? box(V(uniform(0.1f, 2.f), uniform(0.1f, 1.f)), offset)
                                                         : circle(uniform(0.1f, 1.5f), offset);
                const int kind = static_cast<int>(random() % 4);
                if (kind == 0) {
                    spawnStatic(world, position, collider);
                } else {
                    spawnBody(world, position, collider, kind == 1 ? 0.f : 1.f);
                }
            }

            // Brute force: every pair, ordered as the system orders them.
            std::vector<std::tuple<float, ECS::Entity, const Transform*, const Collider*, bool>> all;
            for (auto [entity, transform, collider] : world.view<Transform, Collider>()) {
                all.emplace_back(collider.boundingBox(transform).min.x, entity, &transform, &collider,
                                 world.hasComponents<RigidBody>(entity));
            }
            std::sort(all.begin(), all.end(), [](const auto& a, const auto& b) {
                return std::tie(std::get<0>(a), std::get<1>(a)) < std::tie(std::get<0>(b), std::get<1>(b));
            });
            std::vector<Expected> expected;
            for (std::size_t i = 0; i < all.size(); ++i) {
                for (std::size_t j = i + 1; j < all.size(); ++j) {
                    const auto& [minA, entityA, transformA, colliderA, dynamicA] = all[i];
                    const auto& [minB, entityB, transformB, colliderB, dynamicB] = all[j];
                    if (!dynamicA && !dynamicB) {
                        continue;
                    }
                    if (const auto contact = Component::collide(*colliderA, *transformA, *colliderB, *transformB)) {
                        expected.push_back({entityA, entityB, *contact});
                    }
                }
            }
            const auto byEntities = [](const auto& a, const auto& b) {
                return std::tie(a.first, a.second) < std::tie(b.first, b.second);
            };
            std::sort(expected.begin(), expected.end(), byEntities);

            std::vector<CollisionPair> found = detect(world, systems);
            std::sort(found.begin(), found.end(), byEntities);

            expect(fatal(found.size() == expected.size())) << "seed" << seed;
            expect(expected.size() > 50_u) << "the scene must have contacts to compare";
            bool same = true;
            for (std::size_t k = 0; k < found.size(); ++k) {
                same = same && found[k].first == expected[k].first && found[k].second == expected[k].second
                    && sameContact(found[k].contact, expected[k].contact);
            }
            expect(same) << "seed" << seed;
        }
    };
}
}

void run_collision_system_tests() {
    test_detection();
    test_who_takes_part();
    test_against_every_pair();
}
