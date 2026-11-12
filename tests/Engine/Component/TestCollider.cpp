#include <boost/ut.hpp>

#include <optional>
#include <variant>

#include "ECS/ECS.hpp"
#include "Engine/Common/Collision/Collision.hpp"
#include "Engine/Component/Collider.hpp"
#include "../Support/Approx.hpp"

using namespace boost::ut;
using namespace Engine;
using Component::Collider;
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
    "a default collider is a 1 m box on the Transform that does not bounce"_test = [] {
        const Collider collider;

        expect(std::holds_alternative<Collider::Box>(collider.shape));
        expect(std::get<Collider::Box>(collider.shape).halfSize == V(0.5f, 0.5f));
        expect(collider.offset == V::zero());
        expect(collider.restitution == 0.f);
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

    "the bounding box contains the shape exactly"_test = [] {
        const Transform at{V(1.f, 1.f)};

        expect(box(V(2.f, 1.f)).boundingBox(at) == Collision::Rectangle{V(-1.f, 0.f), V(3.f, 2.f)});
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
        const Collider forms[] = {box(V(0.5f, 0.5f)), circle(0.75f), box(V(1.f, 0.25f), V(0.2f, 0.f)), circle(0.5f, V(0.f, -0.2f))};

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

void run_collider_tests() {
    test_defaults();
    test_world_shape();
    test_collide();
}
