#include "test_framework.hpp"
#include "ECS/Registry.hpp"

int main() {
    ecs_test::run("get_all_entities_returns_a_stable_reference_when_not_dirty", [] {
        ECS::Registry world;
        world.spawnEntity();

        const auto& first = world.getAllEntities();
        const auto& second = world.getAllEntities();

        CHECK(&first == &second);
    });

    ecs_test::run("get_all_entities_reflects_kills_and_spawns_since_the_last_call", [] {
        ECS::Registry world;
        auto e1 = world.spawnEntity();
        auto e2 = world.spawnEntity();
        world.killEntity(e1);

        const auto& afterKill = world.getAllEntities();
        CHECK(afterKill.size() == 1);
        CHECK(afterKill[0].value() == e2.value());

        world.spawnEntity();
        const auto& afterSpawn = world.getAllEntities();
        CHECK(afterSpawn.size() == 2);
    });

    return ecs_test::failure_count() == 0 ? 0 : 1;
}
