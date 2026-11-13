#include <boost/ut.hpp>

#include <cstdint>
#include <optional>
#include <variant>

#include "ECS/ECS.hpp"
#include "Engine/Common/Physic/Collision/Collision.hpp"
#include "Engine/Component/Physics/Collider.hpp"
#include "../../Support/Approx.hpp"

using namespace boost::ut;
using namespace Engine;
using Component::Collider;
using Component::RigidBody;
using Component::Transform;
using TestSupport::near;
using V = Math::Vector2f;

namespace {
constexpr float Tolerance = 1e-6f;

Collider box(V halfSize, V offset = {}) {
    Collider collider;
    collider.shape = Collider::Box{halfSize};
    collider.offset = offset;
    return collider;
}

Collider polygon(const Collider::Polygon& form, V offset = {}) {
    Collider collider;
    collider.shape = form;
    collider.offset = offset;
    return collider;
}

Collider circle(float radius, V offset = {}) {
    Collider collider;
    collider.shape = Collider::Circle{radius};
    collider.offset = offset;
    return collider;
}

bool sameContact(const std::optional<Collision::Contact>& actual, const std::optional<Collision::Contact>& expected) {
    if (actual.has_value() != expected.has_value()) {
        return false;
    }
    return !actual
        || (near(actual->normal, expected->normal, Tolerance) && near(actual->depth, expected->depth, Tolerance)
            && near(actual->point, expected->point, Tolerance));
}

void test_defaults() {
    "a default collider is a 1 m box on the Transform that bounces a little"_test = [] {
        const Collider collider;

        expect(std::holds_alternative<Collider::Box>(collider.shape));
        expect(std::get<Collider::Box>(collider.shape).halfSize == V(0.5f, 0.5f));
        expect(collider.offset == V::zero());
        expect(collider.response == Collider::Response::Bounce);
        expect(collider.restitution > 0.f && collider.restitution < 1.f);
        expect(collider.staticFriction >= collider.kineticFriction);
    };

    "the shapes are usable at compile time"_test = [] {
        constexpr Collider collider;
        constexpr Collision::Rectangle bounds = collider.boundingBox(Transform{V(1.f, 2.f)});
        static_assert(bounds.min == V(0.5f, 1.5f) && bounds.max == V(1.5f, 2.5f));
        expect(bounds.min == V(0.5f, 1.5f));
    };
}

void test_world_shape() {
    "a box is placed on the Transform's position plus the offset"_test = [] {
        const Collider collider = box(V(2.f, 1.f), V(0.f, 1.f));
        const Collider::WorldShape placed = collider.worldShape(Transform{V(3.f, 4.f)});

        expect(collider.center(Transform{V(3.f, 4.f)}) == V(3.f, 5.f));
        expect(fatal(std::holds_alternative<Collision::Rectangle>(placed)));
        expect(std::get<Collision::Rectangle>(placed) == Collision::Rectangle::fromCenter(V(3.f, 5.f), V(2.f, 1.f)));
    };

    "a circle is placed on the Transform's position plus the offset"_test = [] {
        const Collider collider = circle(1.5f, V(-1.f, 0.f));
        const Collider::WorldShape placed = collider.worldShape(Transform{V(3.f, 4.f)});

        expect(fatal(std::holds_alternative<Collision::Circle>(placed)));
        expect(std::get<Collision::Circle>(placed) == Collision::Circle{V(2.f, 4.f), 1.5f});
    };

    "a polygon is its vertices moved to the Transform's position plus the offset"_test = [] {
        const Collider collider = polygon(Collider::Polygon::slope(V(1.f, 0.5f)), V(0.f, 1.f));
        const Collider::WorldShape placed = collider.worldShape(Transform{V(3.f, 4.f)});

        expect(fatal(std::holds_alternative<Collision::Polygon>(placed)));
        expect(std::get<Collision::Polygon>(placed) == Collision::Polygon{V(2.f, 4.5f), V(4.f, 4.5f), V(4.f, 5.5f)});
    };

    "slope builds a right triangle rising to the right or to the left, convex and counter-clockwise"_test = [] {
        constexpr Collider::Polygon right = Collider::Polygon::slope(V(1.f, 0.5f));
        constexpr Collider::Polygon left = Collider::Polygon::slope(V(1.f, 0.5f), false);

        expect(constant<right.points == Collision::Polygon{V(-1.f, -0.5f), V(1.f, -0.5f), V(1.f, 0.5f)}>);
        expect(constant<left.points == Collision::Polygon{V(-1.f, -0.5f), V(1.f, -0.5f), V(-1.f, 0.5f)}>);
        expect(constant<right.points.isConvex() && left.points.isConvex()>);
    };

    "the bounding box contains the shape exactly"_test = [] {
        const Transform at{V(1.f, 1.f)};

        expect(box(V(2.f, 1.f)).boundingBox(at) == Collision::Rectangle{V(-1.f, 0.f), V(3.f, 2.f)});
        expect(polygon(Collider::Polygon::slope(V(1.f, 0.5f))).boundingBox(at) == Collision::Rectangle{V(0.f, 0.5f), V(2.f, 1.5f)});
        expect(circle(0.5f, V(1.f, 0.f)).boundingBox(at) == Collision::Rectangle{V(1.5f, 0.5f), V(2.5f, 1.5f)});
    };

    "a collider stored in the ECS keeps its shape"_test = [] {
        ECS::Registry world;
        const ECS::Entity ball = world.spawnEntity();
        world.addComponent(ball, Transform{V(0.f, 2.f)});
        world.addComponent(ball, circle(0.25f));

        const auto& stored = world.getComponent<Collider>(ball);
        expect(std::get<Collider::Circle>(stored.shape).radius == 0.25f);
        expect(stored.boundingBox(world.getComponent<Transform>(ball)) == Collision::Rectangle{V(-0.25f, 1.75f), V(0.25f, 2.25f)});
    };
}

void test_collide() {
    "collide gives the contact of the two forms, for every pair of forms"_test = [] {
        const Transform atA{V(0.f, 0.f)};
        const Transform atB{V(0.6f, 0.3f)};
        const Collider forms[] = {box(V(0.5f, 0.5f)), circle(0.75f), box(V(1.f, 0.25f), V(0.2f, 0.f)), circle(0.5f, V(0.f, -0.2f)),
                                  polygon(Collider::Polygon::slope(V(0.75f, 0.75f))),
                                  polygon(Collider::Polygon::slope(V(0.5f, 0.5f), false), V(0.f, 0.1f))};

        for (const Collider& a : forms) {
            for (const Collider& b : forms) {
                const std::optional<Collision::Contact> expected = std::visit(
                    [](const auto& first, const auto& second) { return Collision::collide(first, second); },
                    a.worldShape(atA), b.worldShape(atB));

                expect(fatal(expected.has_value()));
                expect(sameContact(collide(a, atA, b, atB), expected));
            }
        }
    };

    "the normal goes from the first collider towards the second"_test = [] {
        const Transform left{V(0.f, 0.f)};
        const Transform right{V(0.8f, 0.f)};

        const auto boxCircle = collide(box(V(0.5f, 0.5f)), left, circle(0.5f), right);
        const auto circleBox = collide(circle(0.5f), right, box(V(0.5f, 0.5f)), left);

        expect(fatal(boxCircle.has_value() && circleBox.has_value()));
        expect(near(boxCircle->normal, V(1.f, 0.f), Tolerance));
        expect(near(circleBox->normal, V(-1.f, 0.f), Tolerance));
        expect(near(boxCircle->depth, 0.2f, Tolerance));
        expect(near(circleBox->depth, 0.2f, Tolerance));
    };

    "the offset moves the shape: colliders apart on their Transforms can touch, and the other way round"_test = [] {
        const Transform atA{V(0.f, 0.f)};
        const Transform atB{V(3.f, 0.f)};

        expect(!collide(box(V(0.5f, 0.5f)), atA, box(V(0.5f, 0.5f)), atB).has_value());
        expect(collide(box(V(0.5f, 0.5f), V(2.2f, 0.f)), atA, box(V(0.5f, 0.5f)), atB).has_value());
        expect(!collide(circle(1.f), atA, circle(1.f, V(1.f, 0.f)), Transform{V(1.5f, 0.f)}).has_value());
    };

    "colliders that do not overlap have no contact"_test = [] {
        const Transform atA{V(0.f, 0.f)};
        const Transform far{V(0.f, 5.f)};

        expect(!collide(box(V(1.f, 1.f)), atA, circle(1.f), far).has_value());
        expect(!collide(circle(1.f), atA, circle(1.f), far).has_value());
        expect(!collide(circle(1.f), far, box(V(1.f, 1.f)), atA).has_value());
    };
}
}

void test_rotation() {
    "turned, a box becomes the polygon of its turned corners; unturned it stays a Rectangle"_test = [] {
        const Collider collider = box(V(1.f, 0.5f));
        const Collider::WorldShape level = collider.worldShape(Transform{V(2.f, 3.f), 0.f});
        const Collider::WorldShape turned = collider.worldShape(Transform{V(2.f, 3.f), 3.14159265f / 2.f});

        expect(std::holds_alternative<Collision::Rectangle>(level));
        expect(fatal(std::holds_alternative<Collision::Polygon>(turned)));
        const Collision::Polygon& corners = std::get<Collision::Polygon>(turned);
        expect(corners.isConvex());
        // A quarter turn: 2 m wide becomes 2 m tall.
        expect(near(corners.boundingBox().min, V(1.5f, 2.f), 1e-5f));
        expect(near(corners.boundingBox().max, V(2.5f, 4.f), 1e-5f));
        expect(near(collider.boundingBox(Transform{V(2.f, 3.f), 3.14159265f / 2.f}).max, V(2.5f, 4.f), 1e-5f));
    };

    "the offset turns with the body; a circle stays a circle, a polygon turns"_test = [] {
        const Transform quarter{V(0.f, 0.f), 3.14159265f / 2.f};
        const Collider ball = circle(0.5f, V(1.f, 0.f));
        expect(near(ball.center(quarter), V(0.f, 1.f), 1e-6f));
        expect(near(std::get<Collision::Circle>(ball.worldShape(quarter)).center, V(0.f, 1.f), 1e-6f));

        const Collider ramp = polygon(Collider::Polygon::slope(V(1.f, 1.f)));
        const Collision::Polygon turned = std::get<Collision::Polygon>(ramp.worldShape(quarter));
        expect(near(turned.vertex(1), V(1.f, 1.f), 1e-6f)) << "(1, -1) turned a quarter";
    };

    "momentOfInertia: the shape's own, moved by the offset (parallel axis)"_test = [] {
        expect(near(box(V(1.f, 0.5f)).momentOfInertia(6.f), 2.5f, 1e-6f));
        expect(near(circle(0.5f).momentOfInertia(4.f), 0.5f, 1e-6f));
        expect(near(circle(0.5f, V(0.f, 2.f)).momentOfInertia(4.f), 0.5f + 16.f, 1e-5f));
        expect(near(box(V(1.f, 0.5f), V(0.f, 2.f)).momentOfInertia(6.f), 2.5f + 24.f, 1e-5f));
        // A slope's vertices are around its box's center, not its centroid: the parallel axis counts it.
        const Collider ramp = polygon(Collider::Polygon::slope(V(1.5f, 1.f)));
        const float own = Physic::polygonInertia(9.f, std::get<Collider::Polygon>(ramp.shape).points);
        const V centroid(0.5f, -1.f / 3.f);
        expect(near(ramp.momentOfInertia(9.f), own + 9.f * centroid.lengthSquared(), 1e-4f));
    };

    "inverseInertia is 0 for what does not turn: fixedRotation, mass 0, no Collider"_test = [] {
        const Collider collider = box(V(1.f, 0.5f));
        RigidBody body{6.f};
        expect(near(Component::inverseInertia(body, &collider), 0.4f, 1e-6f));
        expect(Component::inverseInertia(body, nullptr) == 0.f);
        body.fixedRotation = true;
        expect(Component::inverseInertia(body, &collider) == 0.f);
        expect(Component::inverseInertia(RigidBody{0.f}, &collider) == 0.f);
    };
}

void test_collider_layers() {
    // The layers a game would name, one bit each.
    constexpr std::uint32_t World = 1u << 0;
    constexpr std::uint32_t Player = 1u << 1;
    constexpr std::uint32_t Enemy = 1u << 2;
    constexpr std::uint32_t Ghost = 1u << 3;
    const auto on = [](std::uint32_t layer, std::uint32_t mask) {
        Collider collider;
        collider.layer = layer;
        collider.mask = mask;
        return collider;
    };

    "by default a collider is on layer 1 and collides with every layer"_test = [] {
        constexpr Collider collider;
        static_assert(collider.layer == 1u && collider.mask == ~0u);
        static_assert(Component::canCollide(collider, collider));
        expect(Component::canCollide(collider, collider));
    };

    "the ground accepts several layers at once: the player and the enemy both collide with it"_test = [&] {
        const Collider ground = on(World, Player | Enemy);
        const Collider player = on(Player, World);
        const Collider enemy = on(Enemy, World);

        expect(Component::canCollide(ground, player));
        expect(Component::canCollide(ground, enemy));
        expect(!Component::canCollide(player, enemy)) << "neither accepts the other";
    };

    "both must accept the other: one refusal is enough, even against a collider that accepts everything"_test = [&] {
        const Collider wall = on(World, ~0u);
        const Collider ghost = on(Ghost, ~0u & ~World);
        const Collider player = on(Player, ~0u & ~Enemy);
        const Collider enemy = on(Enemy, ~0u);

        expect(!Component::canCollide(wall, ghost)) << "the ghost refuses World";
        expect(!Component::canCollide(player, enemy)) << "the player refuses Enemy, the enemy accepts Player";
        expect(Component::canCollide(ghost, player));
    };

    "a mask of 0 or a layer of 0 collides with nothing, itself included"_test = [&] {
        const Collider everything = on(~0u, ~0u);
        expect(!Component::canCollide(on(World, 0u), everything));
        expect(!Component::canCollide(on(0u, ~0u), everything));
        expect(!Component::canCollide(on(World, 0u), on(World, 0u)));
    };

    "a collider on several layers is accepted by a mask that accepts any one of them"_test = [&] {
        const Collider both = on(Player | Enemy, World);
        expect(Component::canCollide(both, on(World, Enemy)));
        expect(Component::canCollide(both, on(World, Player)));
        expect(!Component::canCollide(both, on(World, Ghost)));
    };

    "symmetric, and the same as accepting by layer: every layer and mask over 4 bits"_test = [&] {
        bool symmetric = true;
        bool asDefined = true;
        for (std::uint32_t layerA = 0; layerA < 16; ++layerA) {
            for (std::uint32_t maskA = 0; maskA < 16; ++maskA) {
                for (std::uint32_t layerB = 0; layerB < 16; ++layerB) {
                    for (std::uint32_t maskB = 0; maskB < 16; ++maskB) {
                        const Collider a = on(layerA, maskA);
                        const Collider b = on(layerB, maskB);
                        const bool result = Component::canCollide(a, b);
                        symmetric = symmetric && result == Component::canCollide(b, a);
                        // Bit by bit: some layer of a in b's mask, and some layer of b in a's mask.
                        bool bAcceptsA = false;
                        bool aAcceptsB = false;
                        for (std::uint32_t bit = 0; bit < 4; ++bit) {
                            bAcceptsA = bAcceptsA || ((layerA >> bit & 1u) != 0u && (maskB >> bit & 1u) != 0u);
                            aAcceptsB = aAcceptsB || ((layerB >> bit & 1u) != 0u && (maskA >> bit & 1u) != 0u);
                        }
                        asDefined = asDefined && result == (bAcceptsA && aAcceptsB);
                    }
                }
            }
        }
        expect(symmetric);
        expect(asDefined);
    };

    "the 32nd layer works like the first"_test = [&] {
        constexpr std::uint32_t Last = 1u << 31;
        expect(Component::canCollide(on(Last, Last), on(Last, Last)));
        expect(!Component::canCollide(on(Last, ~Last), on(Last, ~0u)));
    };
}

void run_collider_tests() {
    test_defaults();
    test_collider_layers();
    test_rotation();
    test_world_shape();
    test_collide();
}
