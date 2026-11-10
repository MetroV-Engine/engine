// Tests of the editor on an ECS world: ctest -R tests_editor (see README.md).

#include "core/Logger/Logger.hpp"

void run_ecs_scene_tests();
void run_ecs_simulation_tests();
void run_ecs_backend_tests();
void run_editor_on_ecs_tests();
void run_entity_drag_drop_tests();
void run_level_tests();
void run_asset_tests();

int main()
{
    // Expected warnings (a component that cannot be copied...) stay out of
    // the test output; the tests read them from the Logger.
    Logger::instance().setTerminalLevel(LogLevel::Error);

    run_ecs_scene_tests();
    run_ecs_simulation_tests();
    run_ecs_backend_tests();
    run_editor_on_ecs_tests();
    run_entity_drag_drop_tests();
    run_level_tests();
    run_asset_tests();
}
