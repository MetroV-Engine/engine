#include "test_framework.hpp"
#include "ECS/Registry.hpp"

namespace {
    struct Position { float x; float y; };
    struct Velocity { float x; float y; };
}

int main() {
    ecs_test::run("has_components_is_false_before_any_component_is_added", [] {
        ECS::Registry world;
        auto entity = world.spawnEntity();

        CHECK(!world.hasComponents<Position>(entity));
    });

    ecs_test::run("has_components_reflects_components_added_via_addComponent", [] {
        ECS::Registry world;
        auto entity = world.spawnEntity();
        world.addComponent(entity, Position{1.f, 2.f});

        CHECK(world.hasComponents<Position>(entity));
        CHECK(!world.hasComponents<Velocity>(entity));
    });

    ecs_test::run("has_components_requires_all_requested_types", [] {
        ECS::Registry world;
        auto entity = world.spawnEntity();
        world.emplaceComponent<Position>(entity, 1.f, 2.f);

        CHECK((!world.hasComponents<Position, Velocity>(entity)));

        world.emplaceComponent<Velocity>(entity, 0.f, 0.f);

        CHECK((world.hasComponents<Position, Velocity>(entity)));
    });

    ecs_test::run("has_components_clears_after_removeComponent", [] {
        ECS::Registry world;
        auto entity = world.spawnEntity();
        world.addComponent(entity, Position{1.f, 2.f});

        world.removeComponent<Position>(entity);

        CHECK(!world.hasComponents<Position>(entity));
    });

    ecs_test::run("has_components_does_not_leak_across_a_recycled_entity_id", [] {
        ECS::Registry world;
        auto first = world.spawnEntity();
        world.addComponent(first, Position{1.f, 2.f});
        world.killEntity(first);

        auto second = world.spawnEntity();

        CHECK(!world.hasComponents<Position>(second));
    });

    ecs_test::run("has_components_is_false_for_a_dead_entity_handle", [] {
        ECS::Registry world;
        auto entity = world.spawnEntity();
        world.addComponent(entity, Position{1.f, 2.f});
        world.killEntity(entity);

        CHECK(!world.hasComponents<Position>(entity));
    });

    return ecs_test::failure_count() == 0 ? 0 : 1;
}
