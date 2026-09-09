#include "test_framework.hpp"
#include "ECS/Registry.hpp"

namespace {
    struct Position { float x; float y; };
}

int main() {
    ecs_test::run("add_system_does_not_throw_when_component_never_added_yet", [] {
        ECS::Registry world;
        bool ran = false;

        world.addSystem<Position>([&](ECS::Registry& registry) {
            for (auto entry : registry.view<Position>()) {
                (void)entry;
            }
            ran = true;
        });

        world.runSystems();

        CHECK(ran);
    });

    return ecs_test::failure_count() == 0 ? 0 : 1;
}
